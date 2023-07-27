import subprocess
import numpy as np

TEST_TIMEOUT = 20

def run_once(world_size, use_uv, out_file):
    try:
        res = subprocess.run(
            ["./store_barrier", str(world_size), "0", "1" if use_uv else "0"],
            capture_output=True,
            timeout=TEST_TIMEOUT,
        )
    except subprocess.TimeoutExpired as te:
        data = [world_size, use_uv, -2]
    else:
        if res.returncode != 0:
            data = [world_size, use_uv, -1]
        else:
            parts = str(res.stdout).strip().split(" ")
            data = [world_size, use_uv, parts[7]]

    out_file.write(f"{data[0]},{data[1]},{data[2]}\n")
    out_file.flush()
    return int(data[2])

def run_one_config(world_size, use_uv, out_file, repeats):
    data = []
    errors = 0
    timeouts = 0
    for i in range(repeats):
        res = run_once(world_size, use_uv, out_file)
        if res == -1:
            errors += 1
        elif res == -2:
            timeouts += 1
        else:
            data.append(res)
    if len(data) == 0:
        data = [0]
    print(f"ws:{world_size} uv:{use_uv} errors:{errors} timeouts:{timeouts} mean:{np.mean(data):.2f} std:{np.std(data):.2f}")

with open("res.csv", "w+") as f:
    for ws in range(500, 10500, 500):
        run_one_config(ws, False, f, 10)
        run_one_config(ws, True, f, 10)
