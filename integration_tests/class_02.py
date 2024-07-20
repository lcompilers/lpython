from lpython import i32
class Character:
    def __init__(self:"Character", name:str, health:i32, attack_power:i32):
        self.name :str = name
        self.health :i32 = health
        self.attack_power : i32 = attack_power
        self.is_immortal : bool = False
    
    def attack(self:"Character", other:"Character")->str:
        other.health -= self.attack_power
        return self.name+" attacks "+ other.name+" for "+str(self.attack_power)+" damage."

    def is_alive(self:"Character")->bool:
        if self.is_immortal:
            return True
        else:
            return self.health > 0

def main():
    hero : Character = Character("Hero", 10, 20)
    monster : Character = Character("Monster", 50, 15)
    print(hero.attack(monster))
    print(monster.health)  
    assert monster.health == 30
    print(monster.is_alive())  
    assert monster.is_alive() == True
    print("Hero gains temporary immortality")
    hero.is_immortal = True
    print(monster.attack(hero))
    print(hero.health)
    assert hero. health == -5
    print(hero.is_alive())
    assert hero.is_alive() == True
    print("Hero's immortality runs out")
    hero.is_immortal = False
    print(hero.is_alive())
    assert hero.is_alive() == False
    print("Restarting")
    hero = Character("Hero", 10, 20)
    print(hero.is_alive())
    assert hero.is_alive() == True

main()
