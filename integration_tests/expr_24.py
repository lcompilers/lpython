from lpython import i32, f64, TypeVar, Const
from numpy import empty, int32

h = TypeVar("h")
w = TypeVar("w")

def show_img(w: i32, h: i32, A: i32[h, w]):
    print(w, h)
    print(A[0, 0])
    print(A[h - 1, w - 1])

    assert w == 600
    assert h == 450
    assert A[0, 0] == 254
    assert A[h - 1, w - 1] == 254

def show_img_color(w: i32, h: i32, A: i32[h, w, 4]):
    print(w, h)
    print(A[0, 0, 0])
    print(A[h - 1, w - 1, 3])

    assert w == 600
    assert h == 450
    assert A[0, 0, 0] == 214
    assert A[h - 1, w - 1, 3] == 255

def main0():
    Nx: Const[i32] = 600; Ny: Const[i32] = 450; Nz: Const[i32] = 4; n_max: i32 = 255

    xcenter: f64 = f64(-0.5); ycenter: f64 = f64(0.0)
    width: f64 = f64(4); height: f64 = f64(3)
    dx_di: f64 = width/f64(Nx); dy_dj: f64 = -height/f64(Ny)
    x_offset: f64 = xcenter - f64(Nx+1)*dx_di/f64(2.0)
    y_offset: f64 = ycenter - f64(Ny+1)*dy_dj/f64(2.0)

    i: i32; j: i32; n: i32; idx: i32
    x: f64; y: f64; x_0: f64; y_0: f64; x_sqr: f64; y_sqr: f64

    image: i32[450, 600] = empty([Ny, Nx], dtype=int32)
    image_color: i32[450, 600, 4] = empty([Ny, Nx, Nz], dtype=int32)
    palette: i32[4, 3] = empty([4, 3], dtype=int32)

    for j in range(Ny):
        y_0 = y_offset + dy_dj * f64(j + 1)
        for i in range(Nx):
            x_0 = x_offset + dx_di * f64(i + 1)
            x = 0.0; y = 0.0; n = 0
            while(True):
                x_sqr = x ** 2.0
                y_sqr = y ** 2.0
                if (x_sqr + y_sqr > f64(4) or n == n_max):
                    image[j,i] = 255 - n
                    break
                y = y_0 + f64(2.0) * x * y
                x = x_0 + x_sqr - y_sqr
                n = n + 1

    palette[0,0] =   0; palette[0,1] = 135; palette[0,2] =  68
    palette[1,0] =   0; palette[1,1] =  87; palette[1,2] = 231
    palette[2,0] = 214; palette[2,1] =  45; palette[2,2] =  32
    palette[3,0] = 255; palette[3,1] = 167; palette[3,2] =   0

    for j in range(Ny):
        for i in range(Nx):
            idx = image[j,i] - i32(image[j,i]/4)*4
            image_color[j,i,0] = palette[idx,0] # Red
            image_color[j,i,1] = palette[idx,1] # Green
            image_color[j,i,2] = palette[idx,2] # Blue
            image_color[j,i,3] = 255            # Alpha

    print("The Mandelbrot image in color:")
    show_img_color(Nx, Ny, image_color)
    print("The Mandelbrot image in grayscale:")
    show_img(Nx, Ny, image)
    print("Done.")

main0()
