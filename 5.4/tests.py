import subprocess

def run_capacity_branch(branch):
    contents = ""
    with open("btb_capacity_skeleton.c", "r") as btb:
        contents = btb.read()

    loop = ""
    jmps = ""
    nops = ""

    nop_ct = lambda x: x-2

    ct = nop_ct(4) # really a PC difference between labels
    for i in range(ct):
        nops += "NOP "

    for i in range(1, branch):
        loop += f"JMP_LABEL({i})\n"
        jmps += f"JMP({i})\n"


    contents = contents.replace("<label>", loop)
    contents = contents.replace("<nop>", nops)
    # contents = contents.replace("<jmp>", jmps)
    out = f"btb_{branch}_capacity"
    with open(f"{out}.c", "w") as btb:
        btb.write(contents)

    subprocess.run(f"gcc {out}.c -fno-tree-vectorize -march=native -std=c11 -g -o {out}".split(" "))
    output = subprocess.run(f"./{out}", stdout=subprocess.PIPE)
    print(f"{branch},", int(str(output.stdout, "utf-8"))/branch)
    subprocess.run(f"rm {out}".split(" "))
    subprocess.run(f"rm {out}.c".split(" "))


def run_assoc_branch(size, assoc):
    contents = ""
    with open("btb_assoc_skeleton.c", "r") as btb:
        contents = btb.read()

    loop = ""
    jmps = ""
    nops = ""

    nop_ct = lambda x: x-2

    ct = nop_ct(assoc)
    for i in range(ct):
        nops += "NOP "

    for i in range(1, size):
        loop += f"JMP_LABEL({i})\n"
        jmps += f"JMP({i})\n"

    contents = contents.replace("<label>", loop)
    contents = contents.replace("<nop>", nops)
    # contents = contents.replace("<jmp>", jmps)
    out = f"btb_{assoc}_assoc"
    with open(f"{out}.c", "w") as btb:
        btb.write(contents)

    subprocess.run(f"gcc {out}.c -fno-tree-vectorize -march=native -std=c11 -g -o {out}".split(" "))
    output = subprocess.run(f"./{out}", stdout=subprocess.PIPE)

    


if __name__ == "__main__":
    for i in range(256, 8192+256, 256):
        run_capacity_branch(i)




