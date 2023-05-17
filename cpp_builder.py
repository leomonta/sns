#!/usr/bin/env python3
# Building tool for cpp and hpp files
# @Author Leonardo Montagner https://github.com/leomonta/Python_cpp_builder
#
# Build only the modified files on a cpp project
# Link and compile usign the appropriate library and include path
# Print out error and warning messages
# add the args for the link and the compile process
#
# Done: compile and link files
# Done: check for newer version of source files
# Done: skip compilation or linking if there are no new or modified files
# TODO: check for newer versione of header files (check in every file if that header is included, if it has to be rebuilt)
# TODO: identify each header and figure out which source file include which
# Done: error and warning coloring in the console
# Done: if error occurs stop compilation and return 1
# Done: if error occurs stop linking and return 1
# Done: retrive include dirs, libs and args from a file
# Done: retrive target directories for exe, objects, include and source files
# Done: support for debug and optimization compilation, compiler flag and libraries

import subprocess  # execute command on the cmd / bash / whatever
import os  # get directories file names
import json # parse cpp_builder_config.json
from colorama import Fore, init
import hashlib # for calculating hashes
import sys # for arguments parsing


includes_variable = {
	"all_includes" : [],
	"src_referencies": []
}


compilation_variables = {
	# arguments to feed to the compiler and the linker
	"compiler_args": "",
	"linker_args":"",

	# the string composed by the names of the libraries -> "-lpthread -lm ..."
	"libraries_names" : "",

	# the string composed by the path of the libraries -> "-L./path/to/lib -L..."
	"libraries_paths": "",

	# the string composed by the path of the includes -> "-I./include -I./ext/include -I..."
	"includes_paths": "",

	# directories containing the names of the source directories 
	"src_paths" : [],

	# name of the compiler executable
	"compiler_exec": "",

	# base directory of the project
	"project_path":"",

	# directory where to leave the compiled object files
	"objects_path": "",

	# path to the directory where to leave the final executable
	"exe_path": "",

	# name of the final executable
	"exe_name" : ""
}

sha1 = hashlib.sha1()
old_hashes = {}
new_hashes = {}

source_files_extensions = ["c", "cpp", "cxx", "c++", "cc", "C"]


def get_includes():
	global compilation_variables, includes_variable

	for include_directory in compilation_variables["includes_paths"]: # loop trough all the include directories
		for file in os.listdir(include_directory):
			includes_variable["all_includes"].append(file)


def print_stdout(mexage: tuple) -> bool:

	out = mexage[1].split("\n")[0:-1]

	res = True
	
	for i in range(len(out)):
		if "error" in out[i]:
			print(Fore.RED, out[i])
			res = True
		elif "warning" in out[i]:
			print(Fore.BLUE, out[i])
		elif "note" in out[i]:
			print(Fore.CYAN, out[i])
		else:
			print(out[i])

	print(Fore.WHITE)

	return res


def exe_command(command: str) -> tuple:
	"""
	execute the given command and return the output -> [stdout, stderr]
	"""

	stream = subprocess.Popen(command.split(" "), stderr=subprocess.PIPE, universal_newlines=True)

	return stream.communicate()  # execute the command and get the result


def parse_config_json(optimization: bool) -> None:
	"""
	Set the global variables by reading the from cpp_builder_config.json
	"""
	global compilation_variables

	# load and parse the file
	config_file = json.load(open("cpp_builder_config.json"))

	# base directory for ALL the other directories and files
	compilation_variables["project_path"] = config_file["projectDir"]

	# get the compiler executable {gcc, g++, clang, etc}
	compilation_variables["compiler_exec"] = config_file["compilerExe"]

	# --- Libraries path and names ---

	# create the library args -> -lsomelib -lsomelib2 -l...
	if optimization:
		for lname in config_file["libraries"]["Release"]:
			compilation_variables["libraries_names"] += " -l" + lname
	else:
		for lname in config_file["libraries"]["Debug"]:
			compilation_variables["libraries_names"] += " -l" + lname

	compilation_variables["libraries_names"] += compilation_variables["libraries_names"][1:] # remove first whitespace

	# create the libraries path args -> -Lsomelibrary/lib -L...
	for Lname in config_file["Directories"]["libraryDir"]:
		compilation_variables["libraries_paths"] += " -L" + Lname
	compilation_variables["libraries_paths"] = compilation_variables["libraries_paths"][1:] # remove first whitespace

	# --- Include and Source Directories

	# create the includes args -> -Iinclude -Isomelibrary/include -I...
	for Idir in config_file["Directories"]["includeDir"]:
		compilation_variables["includes_paths"] += " -I" + Idir
	compilation_variables["includes_paths"]  = compilation_variables["includes_paths"][1:] # remove first whitespace

	# source dir where the source code file are located
	compilation_variables["src_paths"] = config_file["Directories"]["sourceDir"]

	compilation_variables["objects_path"] = config_file["Directories"]["objectsDir"]
	compilation_variables["exe_path"] = config_file["Directories"]["exeDir"]
	compilation_variables["exe_name"] = config_file["exeName"]

	# --- Compiling an linking arguments ---

	# compiler and linker argument
	if optimization:
		compilation_variables["compiler_args"]  = config_file["Arguments"]["Release"]["Compiler"]
		compilation_variables["linker_args"] = config_file["Arguments"]["Release"]["Linker"]
	else:
		compilation_variables["compiler_args"]  = config_file["Arguments"]["Debug"]["Compiler"]
		compilation_variables["linker_args"] = config_file["Arguments"]["Debug"]["Linker"]


def ismodified(filename: str) -> bool:
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

	global compilation_variables, sha1

	for source_directory in compilation_variables["src_paths"]: # loop trough all the source files directories
		for file in os.listdir(source_directory): # loop trough every file of each directory

			# sha1 hash calculation

			with open(f"{source_directory}/{file}", "r+b") as f:
				sha1.update(f.read())

			# insert in the new_hashes dict the key filename eith the value hash
			new_hashes[source_directory + file] = sha1.hexdigest()  # create the new hash

			# i need to re-instanciate the object to empty it
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


def get_to_compile() -> list:
	"""
	return a list of files and their directories that need to be compiled
	"""

	global compilation_variables

	to_compile = [] # contains directory and filename
	# checking which file need to be compiled
	for source_directory in compilation_variables["src_paths"]: # loop trough all the source files directories
		for file in os.listdir(source_directory): # loop trough every file of each directory
			# i need to differentiate differen parts
			# extension: to decide if it has to be compiled or not and to name it 
			# filename: everything else of the file name ignoring the extension, useful for naming compilitation files
			# source dir: necessary for differentiate eventual same-named files on different dirs

			if ismodified(source_directory + file):

				temp = file.split(".")
				ext = temp.pop(-1)
				file_name = "".join(temp)
				if (ext in source_files_extensions): # check if it is a source file
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


def compile(to_compile: list) -> bool:
	"""
	Compile all correct files with the specified arguments
	"""

	global compilation_variables

	errors = 0

	compiler_exec = compilation_variables["compiler_exec"]
	includes = compilation_variables["includes_paths"]
	compiler_args = compilation_variables["compiler_args"]
	obj_dir = compilation_variables["objects_path"]

	for file in to_compile:
		command = f"{compiler_exec} {compiler_args} {includes} -c -o {obj_dir}/{file[0]}{file[1]}.o {file[0]}/{file[1]}.{file[2]}"
		print(command)
		errors += not print_stdout(exe_command(command))

	return errors > 0


def link() -> bool:
	"""
	Link togheter all the files that have been compiled with the specified libraries and arguments
	"""

	global compilation_variables

	to_link = [] # contains directory and filename
	# checking which file need to be compiled
	for source_directory in compilation_variables["src_paths"]: # loop trough all the source files directories
		for file in os.listdir(source_directory): # loop trough every file of each directory
			# i need to differentiate differen parts
			# extension: to decide if it has to be compiled or not and to name it 
			# filename: everything else of the file name ignoring the extension, useful for naming compilitation files
			# source dir: necessary for differentiate eventual same-named files on different dirs

			temp = file.split(".")
			ext = temp.pop(-1)
			file_name = "".join(temp)
			if (ext in source_files_extensions): # check if it is a source file
				to_link.append([source_directory, file_name, ext])


	compiler_exec = compilation_variables["compiler_exec"]
	linker_args = compilation_variables["linker_args"]
	exe_path = compilation_variables["exe_path"]
	exe_name = compilation_variables["exe_name"]
	libraries_paths = compilation_variables["libraries_paths"]
	obj_dir = compilation_variables["objects_path"]

	Link_cmd = f"{compiler_exec} {linker_args} -o {exe_path}/{exe_name} {libraries_paths}"

	for file in to_link:
		Link_cmd += f" {obj_dir}/{file[0]}{file[1]}.o"
	
	Link_cmd += " " + compilation_variables["libraries_names"]

	print(Link_cmd)
	return print_stdout(exe_command(Link_cmd))


def main():

	global compilation_variables

	if "-o" in sys.argv:
		parse_config_json(True)
	else:
		parse_config_json(False)

	os.chdir(compilation_variables["project_path"])

	#init colorama
	init()

	# create file if it does not exist
	if not os.path.exists("files_hash.txt"):
		f = open("files_hash.txt", "w")
		f.close()


	if "-a" not in sys.argv:
		# load old hashes
		load_old_hashes()

	# obtain new hashes
	calculate_new_hashes()

	# get the file needed to compile
	to_compile = get_to_compile()

	# --- Compiling ---

	print(Fore.GREEN, " --- Compiling ---", Fore.WHITE)

	if not to_compile:
		print("  --- Compilation and linking skipped due to no new or modified files ---")
		return


	# compile each file and show the output,
	# and check for errors
	if compile(to_compile):
		print(f"\n{Fore.RED} --- Linking skipped due to errors in compilation process! ---")
		sys.exit(1)

	# --- Linking ---

	print(Fore.GREEN, " --- Linking ---", Fore.WHITE)

	if not link():
		print(f"\n{Fore.RED} --- Errors in linking process! ---")
		sys.exit(1)

	save_new_hashes()


main()
