#!/usr/bin/env python3
import argparse, json, platform, shutil, subprocess, sys
from pathlib import Path


class Utility:
	
	@staticmethod
	def log(message):
		"""
		Prints a log message to stderr
		"""
		print('[compile.py] {}'.format(message), file=sys.stderr, flush=True)
	
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
	
	@staticmethod
	def capture(command, **kwargs):
		"""
		Executes the specified command and captures its output
		"""
		return Utility.run(
			command, **{'capture_output': True, 'encoding': 'utf-8', **kwargs}
		).stdout.strip()

	@staticmethod
	def delete_recursive(path):
		"""
		Deletes the specified file or directory, performing recursive deletion for directories
		"""
		if path.is_dir():
			shutil.rmtree(path)
		else:
			path.unlink()


def report_missing_engine(source_path):
	"""
	Prints an error message reporting the absence of the required engine source files and then exits
	"""
	Utility.error('\n'.join([
		'the specified path is not the root of a valid Unreal Engine source tree:',
		str(source_path),
		'',
		'Note that only Unreal Engine 5.7 or newer is supported.'
	]), leading_newline=True)


# Parse our command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument('engine_source', help='Path to the root of the Unreal Engine source tree')
parser.add_argument('--wrap', action='store_true', help='Build a container image to wrap the Installed Build')
args = parser.parse_args()

# Resolve the absolute paths to our input directories
script_dir = Path(__file__).parent
quickstart_dir = script_dir.parent
autosdk_dir = quickstart_dir / 'autosdk'
wrap_dir = quickstart_dir / 'wrap-installed-build'
engine_dir = Path(args.engine_source) / 'Engine'

# Verify that the script is running under Linux
if platform.system() != 'Linux':
	Utility.error('This script must be run under Linux. Windows and macOS are not supported.')

# If the script is running under WSL2, verify that the engine source is not located on the host Windows filesystem
if 'microsoft' in platform.release():
	
	if not engine_dir.is_dir():
		report_missing_engine(args.engine_source)
	
	engine_fs = Utility.capture(['stat', '--file-system', '--format="%T"', engine_dir])
	if 'v9fs' in engine_fs:
		Utility.error(' '.join([
			'Cannot mount engine source from a Windows filesystem under WSL2.',
			'Move the engine source into the Linux filesystem and retry.'
		]))

# Attempt to detect the engine version
engine_version = None
version_components = {}
try:
	build_version = engine_dir / 'Build' / 'Build.version'
	version_components = json.loads(build_version.read_text('utf-8'))
	engine_version = '{}.{}.{}'.format(
		version_components['MajorVersion'],
		version_components['MinorVersion'],
		version_components['PatchVersion']
	)
except:
	report_missing_engine(args.engine_source)

# Verify that the engine is a supported version
if version_components['MajorVersion'] < 5 or \
   (version_components['MajorVersion'] == 5 and version_components['MinorVersion'] < 7):
	report_missing_engine(args.engine_source)

# Remove the ADOSupport plugin if it is present, since it breaks compilation under Wine
# (Note that this plugin has been unused for many years and was officially removed in CL 50213900:
#  https://github.com/EpicGames/UnrealEngine/commit/3d5026b1c9f28f99f2c18102f3e60d30b868cc80)
ado_support_plugin = engine_dir / 'Plugins' / 'Runtime' / 'Database' / 'ADOSupport'
if ado_support_plugin.exists():
	Utility.log('Removing ADOSupport plugin: {}'.format(ado_support_plugin))
	shutil.rmtree(ado_support_plugin)

# Build an AutoSDK container image with the appropriate SDK version for the engine
Utility.run(
	[sys.executable, autosdk_dir / 'assemble.py', args.engine_source],
	check=True
)

# Bind-mount the engine source into the AutoSDK container and create an Installed Build
mount_path = '/home/nonroot/.local/share/wineprefixes/prefix/drive_c/UnrealEngine'
Utility.run([
	'docker', 'run', '--rm', '-t', '--init',
	'-v', '{}:{}'.format(args.engine_source, mount_path),
	'-w', mount_path,
	'epicgames/autosdk-wine:{}'.format(engine_version),
	'wine', './Engine/Build/BatchFiles/RunUAT.bat', 'BuildGraph',
	'-script=Engine/Build/InstalledEngineBuild.xml',
	'-target=Make Installed Build Win64',
	'-set:HostPlatformOnly=true'
])

# Wrap the Installed Build in a container image if requested
if args.wrap:
	
	# Clean out any existing contents in the target directory, except for the `.gitignore` file
	target_dir = wrap_dir / 'context' / 'UnrealEngine'
	Utility.log('Removing existing files in {}...'.format(target_dir))
	for child in target_dir.iterdir():
		if child.name != '.gitignore':
			Utility.delete_recursive(child)
	
	# Copy the top-level directories from the Installed Build into the target directory
	Utility.log('Copying Installed Build files to {}...'.format(target_dir))
	installed_build = Path(args.engine_source) / 'LocalBuilds' / 'Engine' / 'Windows'
	for child in installed_build.iterdir():
		if child.is_dir():
			shutil.copytree(child, target_dir / child.name)
	
	# Create the container image
	Utility.run(
		[sys.executable, wrap_dir / 'wrap.py'],
		check=True
	)
