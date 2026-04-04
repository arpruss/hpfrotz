d = {}

with open("font3.txt", "r") as f:
    while True:
        name = f.readline().strip()
        if not name:
            break
        data = b''
        for i in range(14):
            line = f.readline().strip()
            b = int(line.replace(".","0").replace("#","1"),2)
            data += b.to_bytes(1)
        d[name] = data
    
with open("font3.bin", "wb") as f:
    for i in range(128):
        if chr(i) in d:
            data = d[chr(i)]
        else:
            data = b'\0' * 14
        f.write(data)
