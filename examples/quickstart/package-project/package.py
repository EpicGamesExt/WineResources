#!/usr/bin/env python3
import argparse, json, platform, subprocess, sys
from pathlib import Path

class Utility:
	
	@staticmethod
	def log(message):
		"""
		Prints a log message to stderr
		"""
		print('[package.py] {}'.format(message), file=sys.stderr, flush=True)
	
	@staticmethod
	def error(message):
		"""
		Logs an error message and then exits immediately
		"""
		Utility.log('Error: {}'.format(message))
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


def check_v9fs_filesystem(path, description):
	
	if not path.is_dir():
		Utility.error('The {} directory "{}" does not exist.'.format(description, path))
	
	fs_type = Utility.capture(['stat', '--file-system', '--format="%T"', path])
	if 'v9fs' in fs_type:
		Utility.error(' '.join([
			'Cannot mount {} from a Windows filesystem under WSL2.'.format(description),
			'Move the {} into the Linux filesystem and retry.'.format(description)
		]))


# Parse our command-line arguments
parser = argparse.ArgumentParser()
engine_build = parser.add_mutually_exclusive_group(required=True)
engine_build.add_argument('--engine', help='Path to an Installed Build of Unreal Engine to use for packaging')
engine_build.add_argument('--image', help='Tag of a container image encapsulating an Installed Build to use for packaging')
parser.add_argument('--project', required=True, help='Path to the .uproject file of the Unreal Engine project to be packaged')
args = parser.parse_args()

# Resolve the absolute paths to our input directories
script_dir = Path(__file__).parent
autosdk_dir = script_dir.parent / 'autosdk'

# Extract the parent directory and filename of the project
project_path = Path(args.project)
project_dir = project_path.parent
project_file = project_path.name

# Assemble the UAT `BuildCookRun` command to package the project
package_command = [
	'wine', 'C:/UnrealEngine/Engine/Build/BatchFiles/RunUAT.bat', 'BuildCookRun',
	'-project=C:/project/{}'.format(project_file),
	'-archive', '-archivedirectory=C:/project/dist',
	'-platform=Win64', '-clientconfig=Development', '-serverconfig=Development',
	'-nop4', '-allmaps', '-build', '-cook', '-stage', '-package', '-pak', '-iostore', '-compressed'
]

# Verify that the script is running under Linux
if platform.system() != 'Linux':
	Utility.error('This script must be run under Linux. Windows and macOS are not supported.')

# If the script is running under WSL2, verify that the paths to be bind-mounted are not located on the host Windows filesystem
if 'microsoft' in platform.release():
	check_v9fs_filesystem(project_dir, 'project source')
	if args.engine is not None:
		check_v9fs_filesystem(Path(args.engine), 'Installed Build')

# Determine whether we are bind-mounting an Installed Build from the host or using a container image that already wraps a build
if args.engine is not None:
	
	# Ensure we have an AutoSDK container image with the appropriate SDK version for the engine
	Utility.run(
		[sys.executable, autosdk_dir / 'assemble.py', args.engine],
		check=True
	)
	
	# Attempt to detect the engine version (this should succeed if the AutoSDK script above succeeded)
	build_version = Path(args.engine) / 'Engine' / 'Build' / 'Build.version'
	version_json = json.loads(build_version.read_text('utf-8'))
	engine_version = '{}.{}.{}'.format(
		version_json['MajorVersion'],
		version_json['MinorVersion'],
		version_json['PatchVersion']
	)
	
	# Bind-mount both the Installed Build and the project into the AutoSDK container and package the project
	mount_root = Path('/home/nonroot/.local/share/wineprefixes/prefix/drive_c')
	engine_mount = mount_root / 'UnrealEngine'
	project_mount = mount_root / 'project'
	Utility.run([
		'docker', 'run', '--rm', '-it', '--init',
		'-v', '{}:{}'.format(args.engine, engine_mount),
		'-v', '{}:{}'.format(project_dir, project_mount),
		'epicgames/autosdk-wine:{}'.format(engine_version),
		] + package_command
	)
	
else:
	
	# Bind-mount the project into the specified container and package the project
	Utility.run([
		'docker', 'run', '--rm', '-it', '--init',
		'-v', '{}:/home/nonroot/.local/share/wineprefixes/prefix/drive_c/project'.format(project_dir),
		args.image,
		] + package_command
	)
