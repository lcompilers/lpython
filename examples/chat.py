from lpython import i32

def chat():
    name: str = input("Please enter your name: ")

    print("\n----------WELCOME TO THE CHAT----------")
    while True:
        print("\nPlease pick an option:")
        print("1. Say Hi")
        print("2. Say Hello")
        print("3. Say Good Morning")
        print("4. Say Happy Birthday")
        print("5. Exit chat")

        choice: i32 = i32(int(input("Enter your choice: ")))

        if choice == 1:
            print("Hi,", name)
        elif choice == 2:
            print("Hello,", name)
        elif choice == 3:
            print("Good Morning,", name)
        elif choice == 4:
            print("Happy Birthday,", name)
        elif choice == 5:
            print("Exiting chat...")
            break
        else:
            print("Wrong choice. Please try again...")

chat()