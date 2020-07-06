#retrieved from https://stackoverflow.com/questions/33863921/detecting-a-keypress-in-python-while-in-the-background
#objective:
#count W signals sent by ESP32
#display signals

from pyWinhook import HookManager
from pyWinhook import HookConstants
import win32gui
import tkinter

tgtchar = 'W'
count = 0
#main function
def main(event):
    #print(event.Ascii)
    if event.Key is tgtchar:
        global count
        count += 1
    elif event.Ascii is 27:
        print("exiting, storing information\n")
        print("Moved: ",count, " times")
        with open(r'./test.txt', 'a+') as store:
            store.write(" {}".format(count))
        hm.UnhookKeyboard()
        win32gui.PostQuitMessage(0) #breaks loop
    
    return True
#end loop

hm = HookManager()
hm.KeyDown = main
hm.HookKeyboard()

win32gui.PumpMessages() #goto function

print("Hello")