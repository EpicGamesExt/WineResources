#!/usr/bin/env python3
import json, subprocess, sys
from pathlib import Path


class Utility:
	
	@staticmethod
	def log(message, leading_newline=False):
		"""
		Prints a log message to stderr
		"""
		print(
			'[wrap.py]{}{}\n'.format(
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
		Logs and runs a command, returning the exit code
		"""
		stringified = [str(c) for c in command]
		Utility.log(stringified)
		return subprocess.run(stringified, **{'check': True, **kwargs}).returncode


def report_missing_engine(editor_exe):
	"""
	Prints an error message reporting the absence of the required Installed Build files and then exits
	"""
	Utility.error('\n'.join([
		'could not find valid Unreal Engine files!',
		'',
		'Please copy the files for a Win64 Installed Build to:',
		str(unreal_dir),
		'',
		'You can create an Installed Build by building Unreal Engine from source using the script here:',
		str(compile_dir),
		'',
		'Alternatively, you can copy the files from an Installed Build obtained via the Epic Games Launcher:',
		'https://dev.epicgames.com/documentation/en-us/unreal-engine/installing-unreal-engine',
		'',
		'When the files have been copied correctly, the Unreal Editor executable should exist at this path:',
		str(editor_exe),
	]), leading_newline=True)


# Resolve the absolute paths to our input directories
script_dir = Path(__file__).parent
autosdk_dir = script_dir.parent / 'autosdk'
compile_dir = script_dir.parent / 'create-installed-build'
repo_root = script_dir.parent.parent.parent
context_dir = script_dir / 'context'
unreal_dir = context_dir / 'UnrealEngine'
engine_dir = unreal_dir / 'Engine'

# Verify that the Installed Build files have been manually copied to the required location
editor_exe = engine_dir / 'Binaries' / 'Win64' / 'UnrealEditor.exe'
build_version = engine_dir / 'Build' / 'Build.version'
if editor_exe.exists() == False or build_version.exists() == False:
	report_missing_engine(editor_exe)

# Attempt to detect the engine version of the Installed Build
engine_version = None
try:
	version_json = json.loads(build_version.read_text('utf-8'))
	engine_version = '{}.{}.{}'.format(
		version_json['MajorVersion'],
		version_json['MinorVersion'],
		version_json['PatchVersion']
	)
except:
	report_missing_engine(editor_exe)


# Build an AutoSDK base image with the appropriate SDK version for the engine
Utility.run(
	[sys.executable, autosdk_dir / 'assemble.py', unreal_dir],
	check=True
)

# Read the Wine version string so we know the base image tag
wine_version_file = repo_root / 'build' / 'version.json'
wine_version_contents = json.loads(wine_version_file.read_text('utf-8'))
wine_version = wine_version_contents.get('wine-version')

# Build the container image
image_tag = 'epicgames/unreal-engine:dev-wine-{}'.format(engine_version)
Utility.log('Detected files for Unreal Engine version {}'.format(engine_version))
Utility.run([
	'docker', 'buildx', 'build',
	'--progress=plain',
	'--platform', 'linux/amd64',
	'--build-arg', 'UE_VERSION={}'.format(engine_version),
	'-t', image_tag,
	context_dir
])

# Report the build success and print our notice about the lack of a compiler toolchain
print('', file=sys.stderr)
Utility.log('Successfully built container image "{}"'.format(image_tag), leading_newline=True)
