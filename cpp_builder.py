#!/usr/bin/env python3
# Building tool for cpp and hpp files
# @Author Leonardo Montagner https://github.com/leomonta/Python_cpp_builder
#
# Build only the modified files on a cpp project
# Link and compile using the appropriate library and include path
# Print out error and warning messages
# add the args for the link and the compile process
#
# Done: compile and link files
# Done: check for newer version of source files
# Done: skip compilation or linking if there are no new or modified files
# TODO: check for newer version of header files (check in every file if that header is included, if it has to be rebuilt)
# TODO: identify each header and figure out which source file include which
# TODO: if a config value is empty prevent double space in cmd agument
# TODO: add a type config value for gcc | msvc so i can decide which cmd args to use -> -o | -Fo
# Done: added specific linker exec
# TODO: use compiler exec if no linker exec is present
# Done: error and warning coloring in the console
# Done: if error occurs stop compilation and return 1
# Done: if error occurs stop linking and return 1
# Done: retrive include dirs, libs and args from a file
# Done: retrive target directories for exe, objects, include and source files
# Done: support for debug and optimization compilation, compiler flag and libraries

import subprocess  # execute command on the cmd / bash / whatever
import os  # get directories file names
import json  # parse cpp_builder_config.json
import hashlib  # for calculating hashes
import sys  # for arguments parsing


includes_variable: dict[str, list[str]] = {
	# names of all includes dir + name
	"all_includes": [],

	# file: list of references (indices) to include files
	"src_references": {}
}

settings: dict[str, any] = {
	# name of the compiler and linker executable
	"compiler": "",
	"linker": "",

	# compiler and linker args
	"cargs": "",
	"largs" : "",

	# output args
	# output swithces (/Fo -o) for msvc, clang, and gcc
	"oargs": {},


	# path and name of the final executable
	"exe_path_name": "",

	# base directory of the project
	"project_path": "",

	# the string composed by the path of the includes -> "-I./include -I./ext/include -I..."
	"includes": "",

	# directory where to leave the compiled object files
	"objects_path": "",

	# directories containing the names of the source directories
	"source_files": [],


	# the string composed by the names of the libraries -> "-lpthread -lm ..."
	"libraries_names": "",

	# the string composed by the path of the libraries -> "-L./path/to/lib -L..."
	"libraries_paths": "",
}

sha1 = hashlib.sha1()
old_hashes: dict[str, str] = {}
new_hashes: dict[str, str] = {}

source_files_extensions: list[str] = [
	"c",
	"cpp", 
	"cxx",
	"c++",
	"cc",
	"C",
	"s"
]

compilers_common_args: list[dict[str, str]] = [
	{
		"compile_only": "c",
		"output_compiler": "o",
		"output_linker": "o",
		"object_extension": "o"
	},
	{
		"compile_only": "c",
		"output_compiler": "Fo",
		"output_linker": "OUT:",
		"object_extension": "obj"
	}
]


class COLS:

	FG_BLACK = "\033[30m"
	FG_RED = "\033[31m"
	FG_GREEN = "\033[32m"
	FG_YELLOW = "\033[33m"
	FG_BLUE = "\033[34m"
	FG_MAGENTA = "\033[35m"
	FG_CYAN = "\033[36m"
	FG_WHITE = "\033[37m"

	BG_BLACK = "\033[40m"
	BG_RED = "\033[41m"
	BG_GREEN = "\033[42m"
	BG_YELLOW = "\033[43m"
	BG_BLUE = "\033[44m"
	BG_MAGENTA = "\033[45m"
	BG_CYAN = "\033[46m"
	BG_WHITE = "\033[47m"

	FG_LIGHT_BLACK = "\033[90m"
	FG_LIGHT_RED = "\033[91m"
	FG_LIGHT_GREEN = "\033[92m"
	FG_LIGHT_YELLOW = "\033[93m"
	FG_LIGHT_BLUE = "\033[94m"
	FG_LIGHT_MAGENTA = "\033[95m"
	FG_LIGHT_CYAN = "\033[96m"
	FG_LIGHT_WHITE = "\033[97m"

	BG_LIGHT_BLACK = "\033[100m"
	BG_LIGHT_RED = "\033[101m"
	BG_LIGHT_GREEN = "\033[102m"
	BG_LIGHT_YELLOW = "\033[103m"
	BG_LIGHT_BLUE = "\033[104m"
	BG_LIGHT_MAGENTA = "\033[105m"
	BG_LIGHT_CYAN = "\033[106m"
	BG_LIGHT_WHITE = "\033[107m"

	RESET = "\033[0m"


def print_stdout(mexage: tuple) -> bool:

	out = mexage[1].split("\n")[0:-1]

	res = True

	for i in range(len(out)):
		if "error" in out[i]:
			print(COLS.FG_RED, out[i])
			res = False
		elif "warning" in out[i]:
			print(COLS.FG_BLUE, out[i])
		elif "note" in out[i]:
			print(COLS.FG_CYAN, out[i])
		else:
			print(out[i])

	print(COLS.RESET)

	return res


def exe_command(command: str) -> tuple[str, str]:
	"""
	execute the given command and return the output -> [stdout, stderr]
	"""

	stream = subprocess.Popen(command.split(" "), stderr=subprocess.PIPE, universal_newlines=True)

	return stream.communicate()  # execute the command and get the result


def parse_config_json(optimization: str) -> None:
	"""
	Set the global variables by reading the from cpp_builder_config.json
	the optimization argument decide if debug or release mode

	if values are missing from the release section, debug values will be used
	"""

	global settings

	# load and parse the file
	config_file = json.load(open("cpp_builder_config.json"))



	# --- Compiler settings ---
	# get the compiler executable (gcc, g++, clang, etc)
	# and the linker executable, plus the type (needed for cli args)

	settings["compiler"] = config_file["compiler"]["compiler_exe"]

	cstyle: str = config_file["compiler"]["compiler_style"]
	
	# 0 gcc / clang
	# 1 msvc
	compiler_type: int = 0

	if cstyle == "msvc":
		compiler_type = 1
	elif cstyle == "gcc":
		compiler_type = 0
	elif cstyle == "clang":
		compiler_type = 0

	settings["oargs"] = compilers_common_args[compiler_type]

	settings["linker"] = config_file["compiler"]["linker_exe"]



	# --- Directories settings ---
	# Where is the project
	# where are the source files and the include files

	# base directory for ALL the other directories and files
	settings["project_path"] = config_file["directories"]["project_dir"]


	# name of the final executable
	settings["exe_path_name"] = config_file["directories"]["exe_path_name"]


	targets: list[str] = []

	old_dir: str = os.getcwd()

	os.chdir(settings["project_path"])
	for sdir in config_file["directories"]["source_dirs"]:
		for path, subdirs, files in os.walk(sdir):
			for name in files:
				targets.append(f"{path}/{name}")

	os.chdir(old_dir)

	settings["source_files"] = targets


	# create the includes args -> -IInclude -ISomelibrary/include -I...
	for Idir in config_file["directories"]["include_dirs"]:
		settings["includes"] += " -I" + Idir

	settings["objects_path"] = config_file["directories"]["temp_dir"]



	# Optimization dependent stuff
	opt: str = optimization

	# --- Libs ---
	# if the current optimization section has no libs, default to debug
	if not config_file[optimization]["libraries_names"]:
		opt = "debug"

	# create the library args -> -lSomelib -lSomelib2 -l...
	for lname in config_file[opt]["libraries_names"]:
		settings["libraries_names"] += " -l" + lname


	# if the current optimization section has no libs, default to debug
	if not config_file[optimization]["libraries_dirs"]:
		opt = "debug"
	else:
		opt = optimization

	# create the libraries path args -> -LSomelibrary/lib -L...
	for Lname in config_file[opt]["libraries_dirs"]:
		settings["libraries_paths"] += " -L" + Lname



	# --- Compiler an Linker arguments ---

	
	if not config_file[optimization]["compiler_args"]:
		opt = "debug"
	else:
		opt = optimization	
	settings["cargs"] = config_file[opt]["compiler_args"]


	if not config_file[optimization]["linker_args"]:
		opt = "debug"
	else:
		opt = optimization	
	settings["largs"] = config_file[opt]["linker_args"]


	# fix for empty args
	if settings["cargs"]:
		settings["cargs"] = " " + settings["cargs"]

	if settings["largs"]:
		settings["largs"] = " " + settings["largs"]


def is_modified(filename: str) -> bool:
	"""
	Given a filename return if it has been modified
	"""

	global new_hashes
	global old_hashes

	if filename in old_hashes.keys():
		if old_hashes[filename] == new_hashes[filename]:
			return False
	return True


def calculate_new_hashes() -> None:
	"""
	Calculate the hashes for all the source files
	"""

	global settings, sha1

	for file in settings["source_files"]:  # loop trough every file of each directory

		# sha1 hash calculation

		with open(f"{file}", "r+b") as f:
			sha1.update(f.read())

		# insert in the new_hashes dict the key filename with the value hash
		new_hashes[file] = sha1.hexdigest()  # create the new hash

		# i need to re-instantiate the object to empty it
		sha1 = hashlib.sha1()


def load_old_hashes() -> None:
	"""
	Load in old_hashes the hashes present in files_hash.txt
	"""

	global old_hashes

	# read hashes from files and add them to old_hashes array
	with open("files_hash.txt", "r") as f:
		while True:
			data = f.readline()
			if not data:
				break
			temp = data.split(":")

			# remove trailing newline
			temp[1] = temp[1].replace("\n", "")
			old_hashes[temp[0]] = temp[1]


def get_to_compile() -> list[str]:
	"""
	return a list of files and their directories that need to be compiled
	"""

	global settings

	to_compile = []  # contains directory and filename

	# checking which file need to be compiled
	for file in settings["source_files"]:  # loop trough every file of each directory
		# i need to differentiate different parts
		# extension: to decide if it has to be compiled or not and to name it
		# filename: everything else of the file name ignoring the extension, useful for naming compilitation files
		# source dir: necessary for differentiate eventual same-named files on different dirs

		if is_modified(file):

			# get file extension
			temp = file.split(".")
			ext = temp.pop(-1)

			# get filename and relative source dir
			path: list[str] = temp[0].split("/")
			file_name: str = path[-1]
			source_directory: str = "/".join(path[:-1])

			if (ext in source_files_extensions):  # check if it is a source file
				to_compile.append([source_directory, file_name, ext])
	return to_compile


def save_new_hashes() -> None:
	"""
	Write all the hashes on files_hash.txt
	"""

	global new_hashes

	with open("files_hash.txt", "w") as f:
		for i in new_hashes.keys():
			f.write(i + ":")
			f.write(new_hashes[i] + "\n")


def compile(to_compile: list[str]) -> bool:
	"""
	Compile all correct files with the specified arguments
	"""

	global settings

	errors = 0

	cexe = settings["compiler"]
	includes = settings["includes"]
	cargs = settings["cargs"]
	obj_dir = settings["objects_path"]
	oargs = settings["oargs"]

	for file in to_compile:
		obj_name: str = "".join(file[0].split("/"))

		command = f'{cexe}{cargs}{includes} -{oargs["compile_only"]} -{oargs["output_compiler"]} {obj_dir}/{obj_name}{file[1]}.{oargs["object_extension"]} {file[0]}/{file[1]}.{file[2]}'
		print(command)
		errors += not print_stdout(exe_command(command))

	return errors > 0


def link(to_compile: list[str]) -> bool:
	"""
	Link together all the files that have been compiled with the specified libraries and arguments
	"""

	global settings

	to_link: list[str] = []

	for file in settings["source_files"]:  # loop trough every file of each directory

		# get file extension
		temp = file.split(".")
		ext = temp.pop(-1)

		# get filename and relative source dir
		path: list[str] = temp[0].split("/")
		file_name: str = path[-1]
		source_directory: str = "/".join(path[:-1])

		if (ext in source_files_extensions):  # check if it is a source file
			to_link.append([source_directory, file_name, ext])

	lexe = settings["linker"]
	largs = settings["largs"]
	epn = settings["exe_path_name"]
	Libs = settings["libraries_paths"]
	obj_dir = settings["objects_path"]
	oargs = settings["oargs"]

	Link_cmd = f'{lexe}{largs} -{oargs["output_linker"]} {epn}{Libs}'

	for file in to_link:
		obj_name: str = "".join(file[0].split("/"))

		Link_cmd += f' {obj_dir}/{obj_name}{file[1]}.{oargs["object_extension"]}'

	Link_cmd += settings["libraries_names"]

	print(Link_cmd)
	return print_stdout(exe_command(Link_cmd))


def create_makefile():

	# first debug options
	parse_config_json("Debug")
	make_file = ""

	# variables

	make_file += f'CC={compilation_variables["compiler_exec"]}\n'
	make_file += f'BinName={compilation_variables["exe_path"]}/{compilation_variables["exe_name"]}\n'
	make_file += f'ObjsDir={compilation_variables["objects_path"]}\n'

	make_file += "\n# Debug variables\n"
	make_file += f'DCompilerArgs={compilation_variables["compiler_args"]}\n'
	make_file += f'DLinkerArgs={compilation_variables["linker_args"]}\n'
	make_file += f'DLibrariesPaths={compilation_variables["libraries_paths"]}\n'
	make_file += f'DLibrariesNames={compilation_variables["libraries_names"]}\n'

	# first debug options
	parse_config_json(True)

	make_file += "\n# Release variables\n"
	make_file += f'RCompilerArgs={compilation_variables["compiler_args"]}\n'
	make_file += f'RLinkerArgs={compilation_variables["linker_args"]}\n'
	make_file += f'RLibrariesPaths={compilation_variables["libraries_paths"]}\n'
	make_file += f'RLibrariesNames={compilation_variables["libraries_names"]}\n'

	make_file += "\n# includes\n"
	make_file += f'Includes={compilation_variables["includes_paths"]}\n'

	make_file += "\n\n"

	# targets

	os.chdir(compilation_variables["project_path"])

	# obtain new hashes
	calculate_new_hashes()

	# get the file needed to compile
	to_compile = get_to_compile()

	make_file += "debug: DCompile DLink\n\n"

	make_file += "release: RCompile RLink\n\n"

	# Debug commands

	make_file += "\nDCompile: \n"

	for file in to_compile:
		make_file += f"	$(CC) $(DCompilerArgs) $(Includes) -c -o $(ObjsDir)/{file[0]}{file[1]}.o {file[0]}/{file[1]}.{file[2]}\n"

	make_file += "\nDLink: \n"

	make_file += f"	$(CC) $(DLinkerArgs) -o $(BinName) $(DLibrariesPaths)"

	for file in to_compile:
		make_file += f"	$(ObjsDir)/{file[0]}{file[1]}.o"

	make_file += f" $(DLibrariesNames)\n"

	# Release commands

	make_file += "\nRCompile: \n"

	for file in to_compile:
		make_file += f"	$(CC) $(RCompilerArgs) $(Includes) -c -o $(ObjsDir)/{file[0]}{file[1]}.o {file[0]}/{file[1]}.{file[2]}\n"

	make_file += "\nRLink: \n"

	make_file += f"	$(CC) $(RLinkerArgs) -o $(BinName) $(RLibrariesPaths)"

	for file in to_compile:
		make_file += f" $(ObjsDir)/{file[0]}{file[1]}.o"

	make_file += f" $(RLibrariesNames)\n"

	make_file += "\nclean:\n"
	make_file += "	rm -r -f objs/*\n"
	make_file += "	rm -r -f $(BinName)\n"

	with open("Makefile", "w+") as mf:
		mf.write(make_file)


def main():

	global settings


	# makefile option
	if "-e" in sys.argv:
		create_makefile()
		return

 
	# Release or debug mode
	optimization_mode: str = "debug"
	if "-o" in sys.argv:
		optimization_mode = "release"
	
	parse_config_json(optimization_mode)


	# go to the project dir
	os.chdir(settings["project_path"])


	# create file if it does not exist
	if not os.path.exists("files_hash.txt"):
		f = open("files_hash.txt", "w")
		f.close()


	# by not loading old hashes they all look new
	if "-a" not in sys.argv:
		# load old hashes
		load_old_hashes()


	# obtain new hashes
	calculate_new_hashes()


	# get the file needed to compile
	to_compile = get_to_compile()


	# --- Compiling ---

	print(COLS.FG_GREEN, " --- Compiling ---", COLS.RESET)

	# if to_compile is empty, no need to do anything
	if not to_compile:
		print("  --- Compilation and linking skipped due to no new or modified files ---")
		return

	if not os.path.exists("objs"):
		os.makedirs("objs")

	# compile each file and show the output,
	# and check for errors
	if compile(to_compile):
		print(f"\n{COLS.FG_RED} --- Linking skipped due to errors in compilation process! ---")
		sys.exit(1)


	# --- Linking ---

	print(COLS.FG_GREEN, " --- Linking ---", COLS.RESET)

	if not link(to_compile):
		print(f"\n{COLS.FG_RED} --- Errors in linking process! ---")
		sys.exit(1)


	save_new_hashes()


if __name__ == "__main__":
	main()
