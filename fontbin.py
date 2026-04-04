import sys

if len(sys.argv)>2:
    name = sys.argv[2]
else:
    name = "font"

out = "unsigned char "+name+"[] = { ";
with open(sys.argv[1], "rb") as f:
    data = f.read()
for i in range(len(data)):
    if i%32==0:
        out += "\n"
    out += " 0x%02x," % data[i]
out += " };\n";
print(out)
    