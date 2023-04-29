/*
                            *
                            * Project Name:   Biometric Attendance System
                            * Author List:    Adnan Mujawar
                            *     
                            * Filename:       Python Refference
                            * Functionality:  This program can be used on Raspberry py for building a Raspberry pi baised biometric attendance system  
                            * Global Variables: 
                            *
                            */
import sqlite3
import os
import datetime
import time  # used for sleep command
import sys
import pyfingerprint
from pad4pi import rpi_gpio  # for keypad
from pyfingerprint import PyFingerprint
import hashlib
import I2C_LCD_driver
import smtplib
from email.message import EmailMessage


mylcd = I2C_LCD_driver.lcd()

pressed_keys = ''
pin = "1234"
fun = ''
month = ''
year = ''
first_initial = ''
last_initial = ''
student_number = ''
student = ''
r = ''
person = ''


day = datetime.date.today().strftime("%m-%d-%y")  # clean date
timestamp = time.strftime('%I:%M %p')  # is called in function to update


def finger():
    try:
        f = PyFingerprint('/dev/serial0', 57600, 0xFFFFFFFF, 0x00000000)
    except Exception:
        print('The fingerprint sensor could not be initialized!')
        print('')
        main_menu()
    else:
        print('Currently used templates: ' + str(f.getTemplateCount()))
        print('')
        print_grab()


def print_grab():
    global person
    try:
        f = PyFingerprint('/dev/serial0', 57600, 0xFFFFFFFF, 0x00000000)
        mylcd.lcd_clear()
        mylcd.lcd_display_string('*Waiting For Finger*', 2)
        print('*Waiting For Finger*')
        print('')
        ## Wait that finger is read
        while f.readImage() == False:
            pass
        ## Converts read image to characteristics and stores it in charbuffer 1
        f.convertImage(0x01)

        ## Searches template
        result = f.searchTemplate()

        positionNumber = result[0]

        if positionNumber == -1:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   No Match Found', 2)
            print('No match found')
            print('')
            time.sleep(2)
            main_menu()
        else:
            print('Found template at position #' + str(positionNumber))
            print('')
            mylcd.lcd_clear()
            mylcd.lcd_display_string('    PLEASE  WAIT', 2)
            print('PLEASE WAIT')
            print('')

            ## Create Hash Value for finger
            ## Loads the found template to charbuffer 1
            f.loadTemplate(positionNumber, 0x01)
            ## Downloads the characteristics of template loaded in charbuffer 1
            characteristics = str(f.downloadCharacteristics(0x01)).encode(
                'utf-8')
            val_hash = hashlib.sha256(characteristics).hexdigest()
            ## Hashes characteristics of template
            print('SHA-2 hash of template: ' + val_hash)
            print('')
            ## GETTING INFORMATION FROM DATABASE
            conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
            c = conn.cursor()
            c.execute('SELECT * FROM fingerprint_store WHERE id=?',
                      [positionNumber])
            db_val = c.fetchall()
            for row in db_val:
                person = row[0]
                mylcd.lcd_clear()
                mylcd.lcd_display_string('     ID NUMBER:', 2)
                mylcd.lcd_display_string('    ' + person, 3)
                print("YOUR ID NUMBER:" + person)
                print('')
                time.sleep(2)
            conn.commit()
            conn.close()
            check_if_in()
    except Exception as e:
        print(e)


def check_if_in():
    global person
    conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
    c = conn.cursor()
    date = datetime.date.today().strftime("%m-%d-%y")
    c.execute('SELECT status_in FROM students '
              'WHERE first = ? AND date = ?', [person, date])
    conn.commit()
    result = c.fetchone()
    if result is None:
        write_in()
    else:
        check_if_out()


def write_in():
    conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
    c = conn.cursor()
    status_in = 'IN'
    timestamp = time.strftime('%I:%M %p')
    day = datetime.date.today().strftime('%m-%d-%y')
    c.execute("INSERT INTO students (first, date, status_in, time_in) "
              "VALUES(?, ?, ?, ?)", [person, day, status_in, timestamp])
    mylcd.lcd_clear()
    mylcd.lcd_display_string(' Sign In Successful', 2)
    print('Sign In Successful')
    print('')
    time.sleep(2)
    conn.commit()
    conn.close()
    main_menu()


def check_if_out():
    conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
    c = conn.cursor()
    date = datetime.date.today().strftime("%m-%d-%y")
    c.execute('SELECT status_out FROM students '
              'WHERE first = ? AND date = ?', [person, date])
    conn.commit()
    result = c.fetchone()
    convert = ''.join(map(str, result))
    if convert == 'None':
        write_out()
    else:
        mylcd.lcd_clear()
        mylcd.lcd_display_string('  You Have Already', 1)
        mylcd.lcd_display_string('  Signed IN & OUT', 2)
        mylcd.lcd_display_string('       Today', 3)
        print('You have already signed in and out for today')
        print('')
        time.sleep(2)
        main_menu()


def write_out():
    conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
    c = conn.cursor()
    date = datetime.date.today().strftime("%m-%d-%y")
    timestamp = time.strftime('%I:%M %p')
    c.execute("UPDATE students SET status_out = 'OUT' "
              "WHERE first = ? and date = ?", [person, date])
    c.execute("UPDATE students SET time_out = ? "
              "WHERE first = ? and date = ?", [timestamp, person, date])
    mylcd.lcd_clear()
    mylcd.lcd_display_string(' Sign Out Successful', 2)
    print('Sign Out Successful')
    print('')
    time.sleep(2)
    conn.commit()
    conn.close()
    main_menu()

# the following code is for registering a user
# admin data entry
def password():
    global fun
    fun = fun.replace(fun, 'password')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('      Password', 2)
    print("Password")
    print('')


def password_input(key):
    global pressed_keys
    global pin
    if key == '#':
        print(pressed_keys)
        if pressed_keys == pin:
            clear_keys()
            # grad_month()
            admin_menu_1()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string(' Incorrect Password', 2)
            print('Incorrect Password')
            print('')
            time.sleep(2)
            main_menu()
    else:
        pressed_keys += key


def admin_menu_1():
    global fun
    clear_keys()
    mylcd.lcd_clear()
    fun = fun.replace(fun, 'admin_menu_1')
    mylcd.lcd_display_string('1. Register User', 1)
    mylcd.lcd_display_string('2. Email Database', 2)
    mylcd.lcd_display_string('3. More Options' , 3)
    mylcd.lcd_display_string('4. Main Menu', 4)
    print('1. Register Student')
    print('2. E-mail Database')
    print('3. More options')
    print('4. Main Menu')
    print('')


def admin_input(key):
    global pressed_keys
    if key == '#':
        print(pressed_keys)
        if pressed_keys == "1":
            print("send to grad_month")
            print('')
            clear_keys()
            grad_month()
        elif pressed_keys == "2":
            print("send to email db")
            print('')
            clear_keys()
            email()
        elif pressed_keys == "3":
            print("send to more options")
            print('')
            clear_keys()
            admin_menu_2()
        elif pressed_keys == "4":
            print("back to main menu")
            print('')
            clear_keys()
            main_menu()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            admin_menu_1()
    else:
        pressed_keys += key


def email(): 
    msg = EmailMessage()
    msg['Subject'] = 'Attendance Database'
    msg['From'] = 'bshpclinic@gmail.com'
    msg['to'] = 'tonyrodriguezjr@gmail.com'
    msg.set_content('This email contains an a database attachement')

    files = ['/home/pi/Alpha/attendance.db']

    for file in files:
        with open(file, 'rb') as f:
            file_data = f.read()
            file_name = f.name

    msg.add_attachment(file_data, maintype='application',
                       subtype='octet-stream', filename=file_name)

    with smtplib.SMTP_SSL('smtp.gmail.com', 465) as smtp:
        smtp.login('bshpclinic@gmail.com', 'peace777$%')
        smtp.send_message(msg)
    mylcd.lcd_clear()
    mylcd.lcd_display_string('  Database Emailed', 2)
    print('Database Emailed')
    print('')
    time.sleep(2)
    main_menu()


def admin_menu_2():
    global fun
    clear_keys()
    mylcd.lcd_clear()
    fun = fun.replace(fun, 'admin_menu_2')
    mylcd.lcd_display_string('1. Delete User', 1)
    mylcd.lcd_display_string('2. Create Database', 2)
    mylcd.lcd_display_string('3. Delete Database' , 3)
    mylcd.lcd_display_string('4. Previous Menu', 4)
    print('1. Delete User')
    print('2. Create Database')
    print('3. Delete Database')
    print('4. Previous Menu')
    print('')
    
    
def admin_input2(key):
    global pressed_keys
    if key == '#':
        print(pressed_keys)
        if pressed_keys == "1":
            print("send to unenroll user")
            clear_keys()
            unenroll()
        elif pressed_keys == "2":
            print("send to create DB")
            clear_keys()
            create_database()
        elif pressed_keys == "3":
            print("send to delete db")
            clear_keys()
            confirm_del()
        elif pressed_keys == "4":
            print("send back to prevous menu")
            clear_keys()
            admin_menu_1()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            admin_menu_2()
    else:
        pressed_keys += key
        
        
def unenroll():
    ## Tries to initialize the sensor
    try:
        f = PyFingerprint('/dev/serial0', 57600, 0xFFFFFFFF, 0x00000000)

        if ( f.verifyPassword() == False ):
            raise ValueError('The given fingerprint sensor password is wrong!')

    except Exception as e:
        print('The fingerprint sensor could not be initialized!')
        print('Exception message: ' + str(e))
        print('')
        mylcd.lcd_clear()
        mylcd.lcd_display_string('Sensor could not', 2)
        mylcd.lcd_display_string('be initialized', 3)
        time.sleep(2)
        admin_menu_2()

    ## Gets some sensor information
    print('Currently used templates: ' + str(f.getTemplateCount()) +'/'+ str(f.getStorageCapacity()))
    print('')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('Templates: ' + str(f.getTemplateCount()) +'/'+ str(f.getStorageCapacity()), 2)
    time.sleep(2)

    ## Tries to delete the template of the finger
    try:
        mylcd.lcd_clear()
        mylcd.lcd_display_string(' User To Be Deleted', 3)
        positionNumber = input('User To Be Deleted')
        positionNumber = int(positionNumber)
        print('')
        

        if ( f.deleteTemplate(positionNumber) == True ):
            mylcd.lcd_clear()
            mylcd.lcd_display_string('    User Deleted', 2)
            print('User Deleted!')
            print('')
            time.sleep(2)
            admin_menu_2()

    except Exception as e:
        print('Operation failed!')
        print('Exception message: ' + str(e))
        print('')
        mylcd.lcd_clear()
        mylcd.lcd_display_string(' Operation  Failed!', 2)
        time.sleep(2)
        admin_menu_2()
        
        
def create_database():
    #create_db()
    #create_table()
    mylcd.lcd_clear()
    mylcd.lcd_display_string('  Database Created', 2)
    print('Database Created')
    print('')
    time.sleep(2)
    admin_menu_2()
    
    
def confirm_del():
    global fun
    clear_keys()
    mylcd.lcd_clear()
    fun = fun.replace(fun, 'confirm_del')
    mylcd.lcd_display_string('**Delete  Database**', 1)
    mylcd.lcd_display_string('1. Confirm', 2)
    mylcd.lcd_display_string('2. Cancel' , 3)
    print('**Delete  Database**')
    print('1. Confirm')
    print('2. Cancel')
    print('')
    
    
def delete_db(key):
    global pressed_keys
    if key == '#':
        print(pressed_keys)
        if pressed_keys == "1":
            os.remove('attendance.db')
            mylcd.lcd_clear()
            mylcd.lcd_display_string('  Database Deleted', 2)
            print('Database Deleted')
            print('')
            time.sleep(2)
            admin_menu_2()
        elif pressed_keys == "2":
            mylcd.lcd_clear()
            mylcd.lcd_display_string(' Process  Cancelled', 2)
            print('Process Cancelled')
            print('')
            time.sleep(2)
            admin_menu_2()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            admin_menu_2()
    else:
        pressed_keys += key
        
    
def grad_month():
    global fun
    fun = fun.replace(fun, 'month')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('  Graduating Month', 2)
    mylcd.lcd_display_string(' (Two Digit Format)', 3)
    print('Graduating Month')
    print('(Two Digit Format)')
    

def month_input(key):
    global pressed_keys
    global month
    if key == '#':
        months = {'01': 'JAN', '02': 'FEB', '03': 'MAR', '04': 'APR',
                  '05': 'MAY', '06': 'JUN', '07': 'JUL', '08': 'AUG',
                  '09': 'SEP', '10': 'OCT', '11': 'NOV', '12': 'DEC'
                  }
        if pressed_keys in months:
            month = months[pressed_keys]
            clear_keys()
            grad_year()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('  INCORRECT FORMAT', 2)
            print("INCORRECT FORMAT")
            print('')
            time.sleep(2)
            clear_keys()
            grad_month()
    else:
        pressed_keys += key


def grad_year():
    global fun
    fun = fun.replace(fun, 'year')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('  Graduating  Year', 2)
    mylcd.lcd_display_string(' (Four Digit Format)', 3)
    print('Graduating year')
    print('')


def year_input(key):
    global pressed_keys
    global year
    if key == '#':
        if len(pressed_keys) != 4:
            clear_keys()
            mylcd.lcd_clear()
            mylcd.lcd_display_string('  INCORRECT FORMAT', 2)
            print("INCORRECT FORMAT")
            print('')
            time.sleep(2)
            clear_keys()
            grad_year()
        else:
            year = pressed_keys
            clear_keys()
            finitial()
    else:
        pressed_keys += key


def finitial():
    global fun
    fun = fun.replace(fun, 'finitial')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('   First  Initial', 2)
    print('First Initial')
    print('')


def finitial_input(key):
    global pressed_keys
    global first_initial
    if key == '#':
        letters = {'1': 'A', '2': 'B', '3': 'C', '4': 'D', '5': 'E', '6': 'F',
                   '7': 'G', '8': 'H', '9': 'I', '10': 'J', '11': 'K',
                   '12': 'L', '13': 'M', '14': 'N', '15': 'O', '16': 'P',
                   '17': 'Q', '18': 'R', '19': 'S', '20': 'T', '21': 'U',
                   '22': 'V', '23': 'W', '24': 'X', '25': 'Y', '26': 'Z'
                   }
        if pressed_keys in letters:
            first_initial = letters[pressed_keys]
            clear_keys()
            linitial()
        else:
            clear_keys()
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            finitial()
    else:
        pressed_keys += key


def linitial():
    global fun
    fun = fun.replace(fun, 'linitial')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('   Last Initial', 2)
    print('Last Initial')
    print('')


def linitial_input(key):
    global pressed_keys
    global last_initial
    if key == '#':
        letters = {'1': 'A', '2': 'B', '3': 'C', '4': 'D', '5': 'E', '6': 'F',
                   '7': 'G', '8': 'H', '9': 'I', '10': 'J', '11': 'K',
                   '12': 'L', '13': 'M', '14': 'N', '15': 'O', '16': 'P',
                   '17': 'Q', '18': 'R', '19': 'S', '20': 'T', '21': 'U',
                   '22': 'V', '23': 'W', '24': 'X', '25': 'Y', '26': 'Z'
                   }
        if pressed_keys in letters:
            last_initial = letters[pressed_keys]
            clear_keys()
            stnum()
        else:
            clear_keys()
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            linitial()
    else:
        pressed_keys += key


def stnum():
    global fun
    fun = fun.replace(fun, 'stnum')
    mylcd.lcd_clear()
    mylcd.lcd_display_string('   Student Number', 2)
    mylcd.lcd_display_string('    (Two Digits)', 3)
    print('Student Number')
    print('(Two Digits)')
    print('')


def stnum_entry(key):
    global pressed_keys
    global student_number
    if key == '#':
        if len(pressed_keys) != 2:
            clear_keys()
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            stnum()
        else:
            student_number = pressed_keys
            clear_keys()
            confirm()
    else:
        pressed_keys += key


def confirm():
    global fun
    global student
    fun = fun.replace(fun, 'confirm')
    student = first_initial + last_initial + student_number + "-" + month + year
    mylcd.lcd_clear()
    mylcd.lcd_display_string('    Confirm  ID:', 1)
    mylcd.lcd_display_string('    ' + student, 2)
    mylcd.lcd_display_string('1. Confirm', 3)
    mylcd.lcd_display_string('2. Cancel', 4)
    print("ID:", student)
    print("1 : Confirm")
    print("2 : Cancel")
    print('')


def confirm_entry(key):
    global pressed_keys
    global student
    global first_initial
    global last_initial
    global student_number
    global month
    global year
    if key == '#':
        if pressed_keys == '1':
            clear_keys()
            enroll()
        elif pressed_keys == '2':
            first_initial = first_initial.replace(first_initial, '')
            last_initial = last_initial.replace(last_initial, '')
            student_number = student_number.replace(student_number, '')
            month = month.replace(month, '')
            year = year.replace(year, '')
            clear_keys()
            main_menu()
        else:
            clear_keys()
            mylcd.lcd_clear()
            mylcd.lcd_display_string('    INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            confirm()
    else:
        pressed_keys += key


# Registration enrollment process begins here
# Checking db to see if ID already taken
def enroll():
    global student
    global r
    print(student)
    r = student
    print('Student ID:', r)
    print('')
    conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
    c = conn.cursor()
    db_val = c.execute(
        'SELECT student FROM fingerprint_store WHERE student IN (VALUES(?))',
        [r])
    coun = (len(list(db_val)))
    if coun >= 1:
        mylcd.lcd_clear()
        mylcd.lcd_display_string(' ID Already Exists', 2)
        print('ID Already Exists')
        print('')
        time.sleep(2)
        conn.commit()
        conn.close()
        main_menu()
    else:
        conn.commit()
        conn.close()

        try:
            f = PyFingerprint('/dev/serial0', 57600, 0xFFFFFFFF, 0x00000000)

            if (f.verifyPassword() == False):
                print('Contact Admin')
                print('')
                time.sleep(2)
                raise ValueError('The given fingerprint sensor password wrong')

        except Exception as e:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   Contact  Admin', 2)
            print('Contact Admin')
            print('')
            time.sleep(2)
            print('The fingerprint sensor could not be initialized')
            print('Exception message: ' + str(e))
            print('')
            main_menu()

        print('Currently used templates: ' + str(f.getTemplateCount()))
        print('')

        try:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('*Waiting For Finger*', 2)
            print('*Waiting For Finger*')
            print('')

            # Wait for finger to be read

            while (f.readImage() == False):
                pass

            f.convertImage(0x01)

            result = f.searchTemplate()
            positionNumber = result[0]

            if (positionNumber >= 0):
                mylcd.lcd_clear()
                mylcd.lcd_display_string('Fingerprint  Already', 2)
                mylcd.lcd_display_string('       Exists', 3)
                print('Fingerprint Already Exists' + str(positionNumber))
                print('')
                time.sleep(2)
                main_menu()
            else:
                mylcd.lcd_clear()
                mylcd.lcd_display_string('  *Remove  Finger*', 2)
                print('*Remove Finger*')
                print('')
                time.sleep(2)
                mylcd.lcd_clear()
                mylcd.lcd_display_string('*Place Finger Again*', 2)
                print('*Place Finger Again*')
                print('')

                # waiting for second read
                while (f.readImage() == False):
                    pass

                f.convertImage(0X02)

                if (f.compareCharacteristics() == 0):
                    mylcd.lcd_clear()
                    mylcd.lcd_display_string('Fingers Do Not Match', 2)
                    print('Fingers Do Not Match')
                    print('')
                    time.sleep(2)

                f.createTemplate()

                positionNumber = f.storeTemplate()

                f.loadTemplate(positionNumber, 0X01)

                characteristics = str(f.downloadCharacteristics(0x01)).encode(
                    'utf-8')

                cre_hash = hashlib.sha256(characteristics).hexdigest()
                conn = sqlite3.connect('/home/pi/Alpha/attendance.db')
                c = conn.cursor()
                c.execute(
                    'INSERT INTO fingerprint_store (student, hashval, id) '
                    'VALUES (?,?,?)', (r, cre_hash, positionNumber))
                conn.commit()
                conn.close()
                mylcd.lcd_clear()
                mylcd.lcd_display_string('    Fingerprint', 2)
                mylcd.lcd_display_string('    Registered', 3)
                print('Fingerprint Registered In Position' + str(positionNumber))
                print('')
                time.sleep(2)
                admin_menu_1()

        except Exception as e:
            print('Operation failed- Exception message: ' + str(e))
            print('')
            main_menu()


# Keypad stuff

KEYPAD = [
    ["1", "2", "3", "A"],
    ["4", "5", "6", "B"],
    ["7", "8", "9", "C"],
    ["*", "0", "#", "D"]
]

# same as calling: factory.create_4_by_4_keypad, still we put here fyi:
ROW_PINS = [16, 20, 21,
            5]  # BCM numbering; Board numbering is: 7,8,10,11 (see pinout.xyz/)
COL_PINS = [6, 13, 19,
            26]  # BCM numbering; Board numbering is: 12,13,15,16 (see pinout.xyz/)

factory = rpi_gpio.KeypadFactory()

# Try keypad = factory.create_4_by_3_keypad() or
# Try keypad = factory.create_4_by_4_keypad() #for reasonable defaults
# or define your own:
keypad = factory.create_keypad(keypad=KEYPAD, row_pins=ROW_PINS,
                               col_pins=COL_PINS)


def printkey(key):
    print(key)
    
keypad.registerKeyPressHandler(printkey)


# function to clear string
def clear_keys():
    global pressed_keys
    pressed_keys = pressed_keys.replace(pressed_keys, '')


def store_key(key):
    global pressed_keys
    if key == '#':
        print(pressed_keys)
        if pressed_keys == "1":
            clear_keys()
            finger()
        elif pressed_keys == "2":
            print("send to pwd")
            clear_keys()
            password()
        elif pressed_keys == "3":
            print("send to shutdown")
            clear_keys()
            shutdownmenu()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            main_menu()
    else:
        pressed_keys += key
        
        
def shutdownmenu():
    global fun
    clear_keys()
    mylcd.lcd_clear()
    fun = fun.replace(fun, 'shutdownmenu')
    mylcd.lcd_display_string('**Confirm Shutdown**', 1)
    mylcd.lcd_display_string('1. Confirm', 2)
    mylcd.lcd_display_string('2. Cancel' , 3)
    print('**Confirm Shutdown**')
    print('1. Confirm')
    print('2. Cancel')
    print('')
    
    
def confirm_shutdown(key):
    global pressed_keys
    if key == '#':
        print(pressed_keys)
        if pressed_keys == "1":
            print('Send to shutdown')
            print('')
            shutdown()
        elif pressed_keys == "2":
            mylcd.lcd_clear()
            mylcd.lcd_display_string(' Process  Cancelled', 2)
            print('Process Cancelled')
            print('')
            time.sleep(2)
            main_menu()
        else:
            mylcd.lcd_clear()
            mylcd.lcd_display_string('   INVALID  ENTRY', 2)
            print('INVALID ENTRY')
            print('')
            time.sleep(2)
            shutdownmenu()
    else:
        pressed_keys += key
        
        
def shutdown():
    mylcd.lcd_clear()
    mylcd.lcd_display_string('    Shutting down', 1)
    mylcd.lcd_display_string('      [       ]      ', 3)
    time.sleep(.3)
    mylcd.lcd_display_string('      [   *   ]      ', 3)
    time.sleep(.3)
    mylcd.lcd_display_string('      [  ***  ]      ', 3)
    time.sleep(.3)
    mylcd.lcd_display_string('      [ ***** ]      ', 3)
    time.sleep(.3)
    mylcd.lcd_display_string('      [*******]      ', 3)
    time.sleep(.3)
    mylcd.lcd_clear()
    mylcd.lcd_display_string('      Now Safe', 2)
    mylcd.lcd_display_string('    To Shut Down', 3)    
    os.system("sudo shutdown -h now")


# initial options
def boot_sequence():
    mylcd.lcd_clear()
    mylcd.lcd_display_string('       BOOTING', 1)
    mylcd.lcd_display_string('      [       ]      ', 3)
    time.sleep(.1)
    mylcd.lcd_display_string('      [   *   ]      ', 3)
    time.sleep(.1)
    mylcd.lcd_display_string('      [  ***  ]      ', 3)
    time.sleep(.1)
    mylcd.lcd_display_string('      [ ***** ]      ', 3)
    time.sleep(.1)
    mylcd.lcd_display_string('      [*******]      ', 3)
    time.sleep(.1)
    main_menu()


def main_menu():
    global fun
    clear_keys()
    mylcd.lcd_clear()
    mylcd.lcd_display_string('Select, then press #', 1)
    mylcd.lcd_display_string('1. Sign In & Out', 2)
    mylcd.lcd_display_string('2. Admin Menu', 3)
    mylcd.lcd_display_string('3. Shutdown', 4)
    print('1. Sign in & Out')
    print('2. Admin Menu')
    print('3. Shutdown')
    print('')
    fun = fun.replace(fun, 'main_menu')


boot_sequence()


# This function is called each time a keypad button is
def keyHandler(key):
    if fun == 'main_menu':
        store_key(key)
    elif fun == 'password':
        password_input(key)
    elif fun == 'admin_menu_1':
        admin_input(key)
    elif fun == 'admin_menu_2':
        admin_input2(key)
    elif fun == 'confirm_del':
        delete_db(key)
    elif fun == 'month':
        month_input(key)
    elif fun == 'year':
        year_input(key)
    elif fun == 'finitial':
        finitial_input(key)
    elif fun == 'linitial':
        linitial_input(key)
    elif fun == 'stnum':
        stnum_entry(key)
    elif fun == 'confirm':
        confirm_entry(key)
    elif fun == 'shutdownmenu':
        confirm_shutdown(key)


keypad.registerKeyPressHandler(keyHandler)

try:
    while (True):
        time.sleep(0.2)
except:
    keypad.cleanup()
  
