#!/usr/bin/env python3
import argparse, shutil, subprocess, sys
from glob import glob
from pathlib import Path


class VersionConstants:
	
	# The git tag for the version of Wine that we support
	WINE_RELEASE_TAG = 'wine-10.10'
	
	# The corresponding version number for the Wine Ubuntu packages (which we use to identify build dependencies)
	WINE_PACKAGE_VERSION = '10.10'
	
	# The version of Wine Mono used by our supported version of Wine
	# (See: <https://gitlab.winehq.org/wine/wine/-/wikis/Wine-Mono#versions>)
	WINE_MONO_VERSION = '10.1.0'


class Utility:
	
	@staticmethod
	def log(message):
		"""
		Prints a log message to stderr
		"""
		print('[build.py] {}'.format(message), file=sys.stderr, flush=True)
	
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
	def delete_if_exists(path : Path):
		"""
		Deletes the specified file or directory if it exists
		"""
		if path.exists():
			if path.is_dir():
				shutil.rmtree(path)
			else:
				path.unlink()
	
	@staticmethod
	def copy_recursive(source : Path, dest : Path):
		"""
		Copies the specified file or directory, recursively copying any directories
		"""
		if source.is_dir():
			shutil.copytree(source, dest)
		else:
			shutil.copy2(source, dest)
	
	@staticmethod
	def truncate_dir(directory : Path):
		"""
		Ensures the directory exists and is empty
		"""
		Utility.delete_if_exists(directory)
		directory.mkdir(parents=True)
	
	@staticmethod
	def render_template(input : Path, output : Path, context : dict):
		"""
		Renders a template file using the Jinja templating engine
		"""
		
		# Render the input template with Jinja
		from jinja2 import Environment
		environment = Environment(autoescape=False, trim_blocks=True, lstrip_blocks=True)
		template = environment.from_string(input.read_text('utf-8'))
		rendered = template.render(context)
		
		# Remove any excess whitespace
		while '\n\n\n' in rendered:
			rendered = rendered.replace('\n\n\n', '\n\n')
		
		# Ensure the rendered output ends with a single newline
		rendered = rendered.rstrip('\n') + '\n'
		
		# Write the processed text to the output file
		output.write_text(rendered, 'utf-8')


# Resolve the absolute paths to our input directories
script_dir = Path(__file__).parent
template_dir = script_dir / 'template'
context_dir = script_dir / 'context'
dependencies_dir = script_dir / 'dependencies'
patches_dir = script_dir.parent / 'patches'
shim_dir = script_dir.parent / 'memory-shim'
libmemory_patches_dir = script_dir.parent / 'libmemory-patches'

# Verify that our dependencies are installed
try:
	import jinja2
except ModuleNotFoundError:
	Utility.error('\n'.join([
		'required Python packages are not installed. Please run the following command to install them:',
		f'{sys.executable} -m pip install -r {script_dir / "requirements.txt"}'
	]))

# Parse our command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument('--layout', action='store_true', help="Generate the Docker build context but don't build the container image")
parser.add_argument('--no-32bit', action='store_true', help="Don't include 32-bit application support in the container image")
parser.add_argument('--no-sudo', action='store_true', help="Don't give the non-root user sudo privileges in the container image")
parser.add_argument('--no-mitigations', action='store_true', help="Don't include the issue mitigations needed to run UE builds in the container image")
parser.add_argument('--base-image', default='ubuntu:22.04', help="Set the default base image that the container image extends")
parser.add_argument('--user-name', default='nonroot', help="Set the name for the non-root user in the container image")
parser.add_argument('--user-id', default=1000, type=int, help="Set the user ID for the non-root user in the container image")
parser.add_argument('--group-id', default=1000, type=int, help="Set the group ID for the non-root user in the container image")
parser.add_argument('--wine-prefix', default='/home/$USER/.local/share/wineprefixes/prefix', help="Set the filesystem path for the Wine prefix")
args = parser.parse_args()

# Attempt to determine the git commit for the script, falling back to "main" as a sane default
try:
	
	# Retrieve the git hash for the currently checked-out code
	git_output = subprocess.run(
		['git', 'rev-parse', 'HEAD'],
		cwd=script_dir,
		capture_output=True,
		check=True,
		encoding='utf-8',
		text=True
	)
	
	# Verify that we found a non-empty hash
	git_commit = git_output.stdout.strip()
	if len(git_commit) == 0:
		raise FileNotFoundError()
	
except (FileNotFoundError, subprocess.CalledProcessError):
	Utility.log('Warning: failed to determine git commit hash, falling back to "main".')
	git_commit = 'main'

# Construct our Jinja context for processing the Dockerfile template
home_dir = '/home/{}'.format(args.user_name)
options = {
	'BUILD_SCRIPT_VERSION': git_commit,
	'TEMPLATE_WINE_RELEASE_TAG': VersionConstants.WINE_RELEASE_TAG,
	'TEMPLATE_WINE_PACKAGE_VERSION': VersionConstants.WINE_PACKAGE_VERSION,
	'TEMPLATE_WINE_MONO_VERSION': VersionConstants.WINE_MONO_VERSION,
	'TEMPLATE_DEFAULT_BASE_IMAGE': args.base_image,
	'TEMPLATE_ENABLE_32_BIT_SUPPORT': not args.no_32bit,
	'TEMPLATE_ENABLE_SUDO_SUPPORT': not args.no_sudo,
	'TEMPLATE_ENABLE_MITIGATIONS': not args.no_mitigations,
	'TEMPLATE_USER_NAME': args.user_name,
	'TEMPLATE_USER_ID': args.user_id,
	'TEMPLATE_GROUP_ID': args.group_id,
	'TEMPLATE_WINE_PREFIX': args.wine_prefix.replace('$USER', args.user_name),
	'TEMPLATE_CHOWN_DIR': home_dir if args.wine_prefix.startswith('/home/$USER') else '$WINEPREFIX'
}

# Copy the contents of the template directory into the build context directory
# (excluding the Dockerfile template itself, which we process below)
Utility.truncate_dir(context_dir)
for item in glob(str(Path(template_dir / '*'))):
	if not item.endswith('.j2'):
		Utility.copy_recursive(
			Path(item),
			context_dir / Path(item).name
		)

# Render the Dockerfile template using the options specified by the user
Utility.render_template(
	template_dir / 'template.dockerfile.j2',
	context_dir / 'Dockerfile',
	options
)

# Copy the Wine patches into the build context directory
copied_patches = context_dir / 'patches'
Utility.delete_if_exists(copied_patches)
shutil.copytree(patches_dir, copied_patches)

# Copy the Wine patches into the build context directory
copied_shim = context_dir / 'memory-shim'
Utility.delete_if_exists(copied_shim)
shutil.copytree(shim_dir, copied_shim)

# Copy the libmemory-patches into the build context directory
copied_libmemory_patches = context_dir / 'libmemory-patches'
Utility.delete_if_exists(copied_libmemory_patches)
shutil.copytree(libmemory_patches_dir, copied_libmemory_patches)

# Copy the build dependencies into the build context directory
copied_dependencies = context_dir / 'dependencies'
Utility.delete_if_exists(copied_dependencies)
shutil.copytree(dependencies_dir, copied_dependencies)

# Remove the Wine patches README from the build context
Utility.delete_if_exists(copied_patches / 'README.md')

# If mitigations were not enabled then remove the mitigation-specific files from the build context
if args.no_mitigations:
	Utility.delete_if_exists(context_dir / 'msbuild')

# Unless requested otherwise, build the Wine container image
if not args.layout:
	Utility.run([
		'docker', 'buildx', 'build',
		'--progress=plain',
		'--platform', 'linux/amd64',
		'-t', 'epicgames/wine-patched:{}'.format(VersionConstants.WINE_PACKAGE_VERSION),
		context_dir
	])
