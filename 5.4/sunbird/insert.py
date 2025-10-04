import subprocess
import time
def run_test_branch(branch):
    contents = ""
    with open("btb_skeleton.c", "r") as btb:
        contents = btb.read()

    loop = ""
    jmps = ""

    for i in range(1, branch):
        loop += f"JMP_LABEL({i})\n"
        jmps += f"JMP({i})\n"

    contents = contents.replace("<label>", loop)
    # contents = contents.replace("<jmp>", jmps)
    out = f"btb_{branch}_capacity"
    with open(f"{out}.c", "w") as btb:
        btb.write(contents)

    subprocess.run(f"gcc {out}.c -fno-tree-vectorize -march=native -std=c11 -g -o {out}".split(" "))
    output = subprocess.run(f"./{out}", stdout=subprocess.PIPE)
    print(f"{branch},", int(str(output.stdout, "utf-8"))/branch)
    subprocess.run(f"rm {out}".split(" "))
    subprocess.run(f"rm {out}.c".split(" "))


if __name__ == "__main__":
    for i in range(256, 16384+256, 256):
        run_test_branch(i)


