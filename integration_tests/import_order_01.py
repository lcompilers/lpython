from import_order_01b import f
from lpython import i32, ccallback

@ccallback
def main1():
    a: i32 = f()
    print(a)
