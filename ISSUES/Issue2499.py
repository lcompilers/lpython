from lpython import i32, i16, Const
VR_SIZE: i32 = 32_768
l: Const[i32] = VR_SIZE
n: Const[i32] = 15
m: Const[i32] = 3
k: i32
M2: Const[i32] = 5
A_ik: i16
jj: i32
ii: i32
i: i32
for jj in range(0, l, VR_SIZE):  # each VR-col chunk in B and C
    for ii in range(0, n, M2):  # each M2 block in A cols and B rows
        for i in range(0, M2):  # zero-out rows of C
            pass
        for k in range(0, m):  # rows of B
            for i in range(0, M2):
                pass
