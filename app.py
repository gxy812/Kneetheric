#Combined application
#Display count of W pressed
#Count every time W is pressed in the background
#Alert if receive S
#Future achievement?

import tkinter as tk
import datetime as dt
from pyWinhook import *
import win32gui

class Application(tk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.master = master
        #get steps taken from file
        with open(r'./test.txt', 'r') as store:
            try:
                self.iTotal = int(store.read())
            except:
                self.iTotal = 0
        self.iSession = 0
        self.tgtchar = 'W'
        self.errchar = 'S'
        #mutable text value
        self.pointer = tk.StringVar(self)
        self.pack()
        self.create_widgets()
        self.hm = HookManager()
        self.hm.KeyDown = self.onKeyDown
        self.hm.HookKeyboard()
        self.after(250, self.updateDisplay) #run updateDisplay in separate thread

    def create_widgets(self):
        #create text
        self.message = tk.Label(self, textvar = self.pointer)
        self.message.pack(side="top")
        #create alert
        self.alert = tk.Label(self, text = 'Alert', foreground = 'SystemButtonFace')
        self.alert.pack(side='top')

        self.quit = tk.Button(self, text="QUIT", fg="red",
                              command=self.exit)
        self.quit.pack(side="bottom")

    def onKeyDown(self, event):
        if event.Key is self.tgtchar:
            self.iSession += 1
            self.iTotal += 1
            if self.alert['background'] is not 'SystemButtonFace':
                self.alert['background'] = 'SystemButtonFace'
            self.updateDisplay()
        if event.Key is self.errchar and self.alert['background'] is not '#FF0000':
            self.alert['background'] = '#FF0000'
        elif event.Ascii is 27:
            self.exit()
        return True
    def updateDisplay(self):
        self.pointer.set(f'Steps taken: {self.iSession}\nTotal Steps: {self.iTotal}')
        
    def exit(self):
        self.hm.UnhookKeyboard() #stop listening to keypresses
        print("exiting, storing information\n")
        print("Moved: ", self.iSession, " times")
        with open(r'./test.txt', 'w+') as store:
            store.write(f"{self.iTotal}")
        win32gui.PostQuitMessage(0) #breaks loop
        self.master.destroy()

#classname sets title bar
root = tk.Tk(className='Kneetheric')
root.minsize(200, 50)

app = Application(master=root)
app.mainloop()
win32gui.PumpMessages()