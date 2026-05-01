#!/usr/bin/env python3
import argparse, glob, itertools, shutil, subprocess
from pathlib import Path


def capture(command, **kwargs):
	stringified = [str(c) for c in command]
	try:
		return subprocess.run(stringified, **{
			'capture_output': True,
			'check': True,
			'encoding': 'utf-8',
			**kwargs
		})
	except subprocess.CalledProcessError as err:
		raise RuntimeError('command {} failed with exit code {} and stderr:\n{}'.format(
			stringified,
			err.returncode,
			err.stderr
		))

def hash_file(file):
	return capture(['b3sum', '--no-names', file]).stdout.strip()


# Parse our command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument('--outdir', required=True, help='Path to the output directory that will hold the indexed symbol files')
parser.add_argument('indirs', nargs='+', help='Paths to the input directories containing binaries and PDB files')
args = parser.parse_args()
outdir = Path(args.outdir)
indirs = [Path(d) for d in args.indirs]

# Identify all of the PDB and PE files in the input directories
extensions = ['*.dll', '*.pdb']
patterns = itertools.chain.from_iterable([
	[(directory / '**' / extension) for directory in indirs]
	for extension in extensions
])
symbols = itertools.chain.from_iterable([
	glob.glob(str(pattern), recursive=True) for pattern in patterns
])

# Generate the index for each symbol file and move it to the appropriate path under the output directory
skipped = []
warnings = []
for symbol in symbols:
	filename = Path(symbol).name
	index = capture(['symbol-file-indexer', symbol]).stdout.strip()
	destination = outdir / filename / index / filename
	
	# Ignore resource-only DLL files for consistency with the behaviour of Microsoft's `symstore.exe` tool
	if index == 'SENTINEL_IGNORE_FILE':
		skip = f'Skipping resource-only DLL file {symbol}'
		skipped.append(skip)
		print(skip)
		continue
	
	# Skip any duplicate files, but emit a warning if we detect an index collision between files that aren't duplicates
	if destination.exists():
		hash_old = hash_file(destination)
		hash_new = hash_file(symbol)
		if hash_old == hash_new:
			skip = f'Skipping duplicate file {symbol}, existing file at {destination} is identical'
			skipped.append(skip)
			print(skip)
		else:
			warning = f'Warning: detected index collision between non-identical files {symbol} and {destination}'
			warnings.append(warning)
			print(warning)
	else:
		print(f'Moving {symbol} to {destination}...')
		destination.parent.mkdir(parents=True)
		shutil.move(symbol, destination)

# Create an empty file named `pingme.txt` in the root of the output directory, which acts as a sentinel file
(outdir / 'pingme.txt').touch()

# Print a summary of any files that were skipped
print(f'\nTotal files skipped: {len(skipped)}')
print('\n'.join(sorted(skipped)))

# Print a summary of any warnings that were emitted
print(f'\nTotal warnings: {len(warnings)}')
print('\n'.join(sorted(warnings)))
