import os
import subprocess

def change_file_extensions(directory, new_extension):
    for root, _, files in os.walk(directory):
        for file in files:
            old_path = os.path.join(root, file)
            base, ending = os.path.splitext(file)
            if ending != "lox":
                continue
            new_path = os.path.join(root, base + new_extension)
            os.rename(old_path, new_path)
            print(f"Renamed: {old_path} -> {new_path}")

def run_bash_commands(commands):
    for command in commands:
        result = subprocess.run(command, shell=True, text=True, capture_output=True)
        if result.returncode != 0:
            print(f"Command failed: {command}\nError: {result.stderr}")
            return False
    return True

def run_tests(directory, test_command):
    results = {}
    for root, _, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            result = subprocess.run(f"{test_command} {file_path}", shell=True, text=True, capture_output=True)
            
            if result.returncode == 0:
                results[str(file)] = result.stdout.strip()
            else:
                results[str(file)] = result.stderr.strip()
                # print(f"Test failed for: {file_path}\nError: {result.stderr.strip()}")
    return results

def main():
    current_dir = f"{os.getcwd()}/"
    #change_file_extensions(current_dir, ".nz")

    build_commands = [
        "mkdir -p ../build",
        "cd ../build && cmake .. && make",
        "cd ../test"
    ]
    if not run_bash_commands(build_commands):
        print("Build commands failed. Exiting.")
        return

    test_command = "../build/NeZnayu"
    results = run_tests(current_dir + "closure", test_command)

    print("Test results:")
    for file, output in results.items():
        print(f"{file}:\n{output}\n")

if __name__ == "__main__":
    main()
