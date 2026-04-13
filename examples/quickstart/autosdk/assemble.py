#!/usr/bin/env python3
import argparse, json, platform, re, subprocess, sys
from pathlib import Path


class Utility:
	
	@staticmethod
	def log(message, leading_newline=False):
		"""
		Prints a log message to stderr
		"""
		print(
			'[assemble.py]{}{}'.format(
				'\n' if leading_newline == True else ' ',
				message
			),
			file=sys.stderr,
			flush=True
		)
	
	@staticmethod
	def error(message, leading_newline=False):
		"""
		Logs an error message and then exits immediately
		"""
		Utility.log('{}Error: {}'.format(
			'\n' if leading_newline == True else '',
			message
		))
		sys.exit(1)
	
	@staticmethod
	def run(command, **kwargs):
		"""
		Logs and runs a command, verifying that the command succeeded
		"""
		stringified = [str(c) for c in command]
		Utility.log(stringified)
		return subprocess.run(stringified, **{'check': True, **kwargs})


def report_missing_engine(source_path):
	"""
	Prints an error message reporting the absence of the required engine source files and then exits
	"""
	Utility.error('\n'.join([
		'the specified path is not the root of a valid Unreal Engine source tree:',
		str(source_path),
		'',
		'Note that only Unreal Engine 5.6 or newer is supported.'
	]), leading_newline=True)

def parse_windows_sdk_json(windows_sdk_json):
	"""
	Parses the SDK details in `Windows_SDK.json` from an Unreal Engine source tree
	"""
	data = json.loads(Path(windows_sdk_json).read_text('utf-8'))
	
	# Determine the newest version of Visual Studio that the engine supports
	vs_version = None
	for version in reversed(sorted([2022, 2026])):
		if 'MinimumVisualStudio{}Version'.format(version) in data:
			vs_version = version
			break
	
	# Verify that the JSON is not malformed
	if vs_version is None:
		raise
	
	# Retrieve the Visual Studio major version number
	vs_major = data['MinimumVisualStudio{}Version'.format(vs_version)].split('.', 1)[0]
	
	# Retrieve the general list of suggested components/workloads
	suggested = data['VisualStudioSuggestedComponents']
	
	# Retrieve the version-specific list of suggested components/workloads
	suggested += data['VisualStudio{}SuggestedComponents'.format(vs_version)]
	
	# Filter out any components/workloads that we know are not required (e.g. IDE workloads)
	ignored = ['Component.Unreal', 'Microsoft.VisualStudio.Workload']
	filter = lambda component: len([phrase for phrase in ignored if phrase in component]) == 0
	suggested = [component for component in suggested if filter(component)]
	
	# Although the suggested components may list a .NET Framework targeting pack, we actually need the SDK
	is_targeting_pack = lambda c: c.startswith('Microsoft.Net.Component') and c.endswith('TargetingPack')
	suggested = [
		component.replace('TargetingPack', 'SDK') if is_targeting_pack(component) else component
		for component in suggested
	]
	
	# Identify which .NET Framework SDK (if any) we need
	pattern = re.compile(r'Microsoft\.Net\.Component\.(4\.[0-9]+\.[0-9]+)\.SDK')
	matches = [pattern.fullmatch(component) for component in suggested]
	matches = [match.group(1) for match in matches if match is not None]
	dotnet_sdk = matches[0] if len(matches) > 0 else ''
	
	# Verify that we found at least one unfiltered component/workload
	if len(suggested) == 0:
		raise
	
	return {
		'components': suggested,
		'dotnet_sdk': dotnet_sdk,
		'vs_identifier': 'VS{}'.format(vs_version),
		'vs_version': vs_major
	}


# Parse our command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument('engine_source', help='Path to the root of the Unreal Engine source tree')
args = parser.parse_args()

# Resolve the absolute paths to our input directories
script_dir = Path(__file__).parent
repo_root = script_dir.parent.parent.parent
context_dir = script_dir / 'context'
engine_dir = Path(args.engine_source) / 'Engine'

# Verify that the specified engine source path is valid
build_version = engine_dir / 'Build' / 'Build.version'
windows_sdk_json = engine_dir / 'Config' / 'Windows' / 'Windows_SDK.json'
if build_version.exists() == False or windows_sdk_json.exists() == False:
	report_missing_engine(args.engine_source)

# Attempt to detect the engine version
engine_version = None
try:
	version_json = json.loads(build_version.read_text('utf-8'))
	engine_version = '{}.{}.{}'.format(
		version_json['MajorVersion'],
		version_json['MinorVersion'],
		version_json['PatchVersion']
	)
except:
	report_missing_engine(args.engine_source)

# Attempt to parse the SDK details in `Windows_SDK.json`
sdk_details = None
try:
	sdk_details = parse_windows_sdk_json(windows_sdk_json)
except:
	report_missing_engine(args.engine_source)

# Build our Wine base image with the default options
# (This ensures mitigations are included, which are needed for .NET and C++ compilation)
build_script = 'build.bat' if platform.system() == 'Windows' else 'build.sh'
Utility.run([repo_root / 'build' / build_script])

# Read the Wine version string so we know the base image tag
wine_version_file = repo_root / 'build' / 'version.json'
wine_version_contents = json.loads(wine_version_file.read_text('utf-8'))
wine_version = wine_version_contents.get('wine-version')

# Build our AutoSDK image
image_tag = 'epicgames/autosdk-wine:{}'.format(engine_version)
Utility.run([
	'docker', 'buildx', 'build',
	'--progress=plain',
	'--platform', 'linux/amd64',
	'--build-arg', 'COMPONENTS_AND_WORKLOADS={}'.format(' '.join(sdk_details['components'])),
	'--build-arg', 'DOTNET_FRAMEWORK_SDK={}'.format(sdk_details['dotnet_sdk']),
	'--build-arg', 'VISUAL_STUDIO_IDENTIFIER={}'.format(sdk_details['vs_identifier']),
	'--build-arg', 'VISUAL_STUDIO_VERSION={}'.format(sdk_details['vs_version']),
	'--build-arg', 'WINE_VERSION={}'.format(wine_version),
	'-t', image_tag,
	context_dir
])

# Report the build success
print('', file=sys.stderr)
Utility.log('Successfully built container image "{}"'.format(image_tag), leading_newline=True)
