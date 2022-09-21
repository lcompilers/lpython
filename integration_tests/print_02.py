from ltypes import i32, f64

# Test: Printing ListConstant
def f():
    a: list[str] = ["ab", "abc", "abcd"]
    b: list[i32] = [1, 2, 3, 4]
    c: list[f64] = [1.23 , 324.3 , 56.431, 90.5, 34.1]
    d: list[i32] = []

    print(a)
    print(b)
    print(c)
    print(d)

    # print list constant
    print([-3, 2, 1, 0])
    print(['a', 'b', 'c', 'd' , 'e', 'f'])

def test_nested_lists():
    w: list[list[list[list[list[f64]]]]] = [[[[[2.13, -98.17]]], [[[1.11]]]]]
    x: list[list[list[i32]]] = [[[3, -1], [-2, 5], [5]], [[-3, 1], [2, -5], [-5]]]
    y: list[list[f64]] = [[3.14, -1.0012], [-2.38, 5.51]]
    z: list[list[str]] = [["bat", "ball"], ["cat", "dog"], ["c", "c++", "java", "python"]]

    print(w)
    print(x)
    print(y)
    print(z)

def test_nested_lists2():
    # It tests list printing on scale like lists of size (approx) 100.

    # shape = (10, 10), values range from 0 to 99 (both inclusive)
    p: list[list[i32]] = [[0, 1, 2, 3, 4, 5, 6, 7, 8, 9], [10, 11, 12, 13, 14, 15, 16, 17, 18, 19], [20, 21, 22, 23, 24, 25, 26, 27, 28, 29], [30, 31, 32, 33, 34, 35, 36, 37, 38, 39], [40, 41, 42, 43, 44, 45, 46, 47, 48, 49], [50, 51, 52, 53, 54, 55, 56, 57, 58, 59], [60, 61, 62, 63, 64, 65, 66, 67, 68, 69], [70, 71, 72, 73, 74, 75, 76, 77, 78, 79], [80, 81, 82, 83, 84, 85, 86, 87, 88, 89], [90, 91, 92, 93, 94, 95, 96, 97, 98, 99]]

    # shape = (3, 3, 3, 3), values range between [-101.00, 101.00)
    q: list[list[list[list[f64]]]] = [[[[  74.55,  -77.64,   52.35],
         [ -78.25,  -19.24,   81.38],
         [   7.86,   12.11,   27.5 ]],

        [[ -98.93,  -79.62,  -73.76],
         [  42.05,  -90.29,   69.95],
         [  59.32,  -70.78,  -53.22]],

        [[  53.52,  -93.88,   49.65],
         [   2.18,   19.91,   69.24],
         [  78.51,   89.69,  -86.68]]],


       [[[ -92.48,  -80.75,  -27.76],
         [ -13.35,   12.28,   79.61],
         [  48.48,  -10.49,   41.1 ]],

        [[  20.33,   14.52,   82.56],
         [ -57.76,   76.03,   67.33],
         [ -21.5 ,  -11.92,  -31.84]],

        [[   9.65,   11.15,  -36.58],
         [  56.55,  -70.85,  -50.67],
         [  94.63,   25.68,   89.3 ]]],


       [[[  28.53,   71.22,   71.87],
         [ -58.39,   -6.96,   42.98],
         [  -8.95,  -75.09,   37.96]],

        [[ -31.75,   67.33,   58.17],
         [ -64.01,   -9.22,    9.59],
         [  15.71,   82.36,   51.18]],

        [[-100.29,  -32.72,  -54.94],
         [  -0.32,   68.81,  -55.09],
         [  97.28,  -28.2 ,  -62.61]]]]

    # shape = (5, 5, 5), where each element is a string of random alphanumerals and length betweem 0 and 10
    r: list[list[list[str]]] = [[['Io', 'tl', 'bLvjV', 'wjFKQ', 'lY2'], ['Be2l6bqE', 'pQER3utIXA', 'llZBJj5Cdu', 'C8', 'gwTr77PdYR'], ['4M6L', 'ktPdowqERy', 'KSifqTkR', 'ZE2p1N78f1', 'Mi5e87Xw'], ['uwfzqDq9g', 'QaM1s', '', '', 'LB'], ['OJFRY6k', 'iz7Oie', '', 'LUYLF', 'JBND5FuV7l']], [['m', 'WIQBQfV', 'jxjDrqxu', 'kea', 'mu'], ['', 'GI8aOwLMe', 'Y5m8', 'a02Rz', 'xNKCJ'], ['LzkhyiJQHP', 'uzc3xyoXL', 'sKGnYfpRy', '7x', 'WTVKrnPO'], ['TZa6', 'GXRuyRX', 'R', 'JQxS', 'OH'], ['bSVJZ1OQ', 'M', 'I9omlF', 'x7FR', 'XtpL']], [['DKOpK', 'eg8Nz', 'ru', 'Sj', 'YUDxyI'], ['Q5uyhvp', 'Ydx', 'p', 'DLM5RX', 'pwOujxCO'], ['s5GOWnNJV', 'af', 'KAkD', '4IIZK', 'JQK040x'], ['9vF', '9pc7R8v', 'nDReIU7', 'K', 'btn'], ['', 'wVeivkdi', '', '', 'C']], [['vNTtcRXD', 'rsi', 'YsoF7mZD', 'VrPXU50rgA', 'mG7zqN0G'], ['la7cJ', 'M5rLJ8Go', 'gb', 'FjKwYZ7E', 'uSPD'], ['', 'oOa79jWcMx', 'yyAYZZ', 'wbvggXm', 'aE3BkCL4'], ['RdP', 'Hwc0x9RZ', 'sy', '9', 'W1d9xA2BXe'], ['A', '', 'QnK', 'N5tzN', 'ou7Lp']], [['DL68rDF', 'v', 'kQ3Mxm', 'g', '6KTeF4Eo'], ['Hx9', 'Y1IzQm85Z4', '3D8', 'ZLZ5', 'rWn'], ['LtT', 'Dh5B', 'M', 'F', 'QTARbY'], ['Sh', 'WL', 'yvAfWvZSx1', '90yx', 'v'], ['', '7IBW', 'nI', '', '6Cbp5c8RT']]]

    print(p)
    print(q)
    print(r)

f()
test_nested_lists()
test_nested_lists2()
