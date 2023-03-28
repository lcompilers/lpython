from lpdraw import Line, Circle, Clear, Display, DisplayTerminal

def main():
    Clear()
    Line(2, 4, 99, 11)
    Line(0, 39, 49, 0)
    Circle(52, 20, 6.0)
    Display()
    DisplayTerminal()

main()
