1:Whats this?
2:Install
3:BIOS Setup
4:Paths
5:Play
6:Controls
7:fAQ
8.Authors


<1: What's this?>
fMSX is the MSX/MSX2/MSX2+ emulator by Marat Fayzullin.
It runs MSX/MSX2/MSX2+ software with very fast on many different platforms including Windows, MacOS, Unix,
MSDOS, AmigaOS, etc.
See http://fms.komkon.org/fMSX/ for further informations.

fMSX3DS is the port of fMSX for Nintendo3DS(New 3DS/New 2DS/Old 3DS/Old 2DS).
In additional, it add audio emulation by Mitsutaka Okazaki(emu2413.c etc.) with latest version.
 It's not the legacy version of over 20 year ago that many emulator uses, but latest(2022) one.
In additional, it add MSXTurboR and MSX0 emulation.
Also, it add various improvements based on recently analize of MSX hardware
include analize in Japan that is unknown in world wide.
Read "What's new.txt" to see more detail.


<2: Install>
CIA vertsion: Install "fMSX3DS.cia" with your favorite CIA installer.(i.e. FBI).

Homebrew Launcher: Copy "fMSX3DS.3dsx" to "3DS" folder("root\3DS") on your SD card.

Both CIA version and Homebre Launcher version need "dsp.firm" file in the "3DS" folder("root\3DS") on your SD card to enable sound.
If not, you can install that with Luma3DS.
To install this, Press Left Shoulder Trigger key, D-pad Down key and Select key at the same time to enter Rosalina Menu.
And select "Miscellaneous options" to enter Miscellaneous options menu and select "Dump DSP Firmware".


<3: BIOS Setup>
If you are using MSXDS, fMSX core for retroarch, or BlueMSX core for retroarch,
fMSX3DS load these BIOS files directly and no need for setup BIOS files.
If not, Copy following MSX BIOS files to the ROOT of your SD card or "FMSX3DS" folder("root\FMSX3DS").
MSX.ROM
MSX2.ROM
MSX2EXT.ROM
MSX2P.ROM 
MSX2PEXT.ROM

additionally i recommend you to use these files.
DISK.ROM			:To play games with disk image.
KANJI.ROM			:Display Japanese KANJI.
FMPAC.ROM			:To enable FM Music and SRAM save.
A1WXKDR.ROM			:KANJI BASIC. Some MSX2+ games need this.
				 Also, Need for Show up MSX2+ bootup screen.

As to "A1WXKDR.ROM", you can replace that with the "MSXKANJI.ROM" or "KNJDRV.ROM".

If you want to play the MSXTurboR you need these files.
MSXTR.ROM
MSXTREXT.ROM
MSXTROPT.ROM
MSXKANJI.ROM (A1WXKDR.ROM, KNJDRV.ROM)
MSXDOS23.ROM
MSXTRMUS.ROM

If you miss BIOS, fMSX3DS use the C-BIOS.
http://cbios.sourceforge.net/
C-BIOS is the open source MSX BIOS by BouKiCHi, and many developers modified that.
You can play almost ROM games with C-BIOS, but it doesn't support Disk games, nor support MSXTurboR,
nor support Programming with MSX BASIC.

If you miss KANJI.ROM, fMSX3DS use pseudo KANJI.ROM from A to C.
http://www.yo.rim.or.jp/~anaka/AtoC/labo/labo32.htm


<4:Paths>
fMSX3DS creates these folders.
/FMSX3DS/SAVEDISK		:Saved Disk.
/FMSX3DS/SNAP			:Screen Shot Image.
/FMSX3DS/SRAM			:S-RAM Save Data.
/FMSX3DS/STATE			:Saved State
/FMSX3DS/TAPE			:Saved Cassette Tape


<5:Play>
When you start fMSX3DS, it show select file dialog.
You can select ROM cartridge, Disk image, Cassette Tape etc.
fMSX3DS supports files with extension ".ROM", ".MX1", ".MX2", ".DSK", ".CAS".
Also, It supports ".ZIP" or ".gz" compressed files.


<6:Controls>
Default Key Mapping:
A:Fire A	B:Fire B	X:"x" key	Y:"m" key	L:F1 key	R:F2 key
Select:F5	ZL:control key	ZR:graph key
Start:fMSXD3DS Menu
You can change key mapping with [Key Config] menu in the fMSX3DS menu.

Software Keyboard:
-Touching the keyboard icon on the lower right of the bottom screen enable you to using Software Keyboard.

Mouse:
-Touching the mouse icon on the lower right of the bottom screen enable you to using mouse.
Then, you can move mouse with touch screen, and you can press mouse button with L key or R key.

Arkanoid Paddle:
-Arkanoid paddle emulation starts automatically when you load Arkanoid/Arkanoid II.
Then you can move paddle with touch screen, and you can shot fire with L key or R key.


<7.Special Hardware>
fMSX3DS is first emulator that support +PCM hardware and VoiceBox and MSX0 hardware.
Also, fMSX3DS supports various special hardwares.
+PCM:
+ PCM is ADPCM Voice Sampling hardware.
No commercial game support this hardware. but, some japanese homebrew/doujin game support this.
Especially noteworthy, Pleasure Hearts (the legendary homebrew made by author of Judgement Silversword, ESCHATOS)
support this!
http://hp.vector.co.jp/authors/VA011751/MSXSR8-2.HTM
To use this, Choose "[Option]" item in the fMSX3DS system menu,
and change option item "<Input/Output Hardware Option>/Printer Port" to "PCM+".

VoiceBox:
VoiceBox is PCM Voice Sampling hardware.
As to commercial games, Only one game (Isseki ni kakeru Seishun by LOG) support this hardware.
http://hirosedou.sblo.jp/article/93257998.html
To use this, Choose "[Option]" item in the fMSX3DS system menu,
and change option item "<Input/Output Hardware Option>/Printer Port" to "Voice Box".

stereoscopic 3D Glass:
Some MSX games and test programs use stereoscopic 3D.
For example, Dim X(Kai Magazine) support this.
https://www.msxgamesworld.com/software.php?id=4752
fMSX3DS emulate this with Nintendo3DS's stereoscopic 3D.
To use this, First, Move 3D depth slider up to enable Nintendo 3DS's stereoscopic 3D,
then, choose "[Option]" item in the fMSX3DS system menu,
and change option item "<Graphic Setting/Use 3D Stereoscopic> to select stereoscopic 3D mode.
 Currently,you can select only anaglyph 3D and anaglyph 3D(Color) for now.

V9990:
Some homebrew game/app requires V9990 or ites clones(GFX9000 etc).
To use that, Chose "[Option]" item in the fMSX3DS system menu, and enable the "Use V9990" option item.

MSX0:
MSX0 is new offical MSX project start at 2023.
https://camp-fire.jp/projects/view/648742
MSX0 can connect many kind of IOT devices and control these with MSX
,and many people make homebrew or doujin game/app with this feature.
To use MSX0 with fMSX3DS, Choose "[Option]" item in the fMSX3DS system menu, and enable the "Use MSX0" option item.

fMSX3DS use fake MSXIOT ROM file for MSX0 and you need no MSXIOT ROM file.
But, some MSX0 games/apps requires MSX-BASIC KUN(X-BASIC) ROM.
If you are using BlueMSX core for retroarch, fMSX3DS load it's ROM files directly and no need for setup X-BASIC ROM files.
If not, Rename MSX-BASIC KUN PLUS(X-BASIC2) ROM to "MSX0_XBASIC.ROM" or "XBASIC2.rom" and copy that to the ROOT of your SD card
 or "FMSX3DS" folder("root\FMSX3DS").
Then, Choose "[Option]" item in the fMSX3DS system menu, and enable the "<MSX0 Option>/Load X-BASIC2" option item.

As to touch screen of MSX0, you can use that with touching the mouse icon on the lower right of the bottom screen when you enabled
the "Use MSX0" option item in the option menu.


<8.FAQ>
Q.I cann't run fMSX3DS!
A.Try to delete "fMSX3DS.cfg" in the "FMSX3DS"folder("root\FMSX3DS").
 Replace "CMOS.ROM" with the one in the "fMSX3DS.zip" file.

Q.My 3DS freeze!
A.Press power button for 10 second.

Q.Does fMSX3DS work on Old3DS?
A.Yes.fMSX3DS works full speed even on Old3DS, but that has fllowing limitations.
If you dislike this limitations, use New3DS that has no limitation.

-Old2DS does'nt support Nintendo3DS's 800px wide mode.
 Other hardware (Old3DS, New 3DS and New 2DS) support that.

-When you enable interlace, Screen flicker appears on Old3DS/Old2DS.

-Enabling FM Sound reduce emulation speed on Old3DS/Old2DS.

-Slow on MSXTurboR.On New3DS it runs full speed.

Q.A disk that was once working suddenly stopped working. What should I do?
A.Try to delete disk file with the same name that has placed "FMSX3DS/SAVEDISK" folder("root\FMSX3DS\SAVEDISK").
 Caution that when you delete the file, you lost save data. So i suggest you to make backup of that file.

Q.What's the point of "[Load Screen Shot]" item in the fMSX3DS system menu?
A.It shows a image taht was made with "[Save Screen Shot]" menu.
It helps you to manage passwords for games's ave data.

Q.What's point of "[Load Screen Shot](Show Keyboard)" item in the fMSX3DS system menu?
A.It shows a image taht was made with "[Save Screen Shot]" menu.
 Additionally, it also shows the keyboard at the same time. So, you can use software keyboard with referencing a image file.
 It helps you to manage passwords for game's save data(with keyboard input).
 (The Maze of Galious (Knightmare II), Dragon Slayer 4, Gekitotsu Pennant Race etc.)
 Also, It helps you to programming with referencing a image of old computer magazine.

Q.What's the point of "[Load Konami SCC+ ROM]" item in the fMSX3DS system menu?
A.Some disk gmaes requires konami SCC Plus ROM cartridge in the MSX's slot.(Snatchr, SD Snatcher, 
 Konamai Game collections, Many  kids of Disk Magazines/Music Disks etc).
 This menu enable you to play these disks with inserting the virtual SCC Plus cartridge.

Q.What's the point of "Force Japanese BIOS" item in the [Option] menu?
A.Load Japanese BIOS named "MSXJ.ROM" or "MSX2J.ROM" as match as posible.
 It's usefull for playing a game that works only japanese MSX(metal gear etc.).
 Also It's usefullfor playing games with 60 FPS.

Q."[State Load]" item in the fMSX3DS system menu does'nt work.
A. To load the saved state, the RAM size must be the same size as when it was saved.
Revert RAM Size with "RAM Size" option item in the [option] menu. 


<9.Authors>
fMSX : MSX computer emulator
 http://fms.komkon.org/fMSX/

  Original fMSX                  by Marat Fayzullin (1994-2021).
  fMSX-SDL port                  by Vincent van Dam (2001).
  YM2413/PSG/SCC/Y8950 emulation by Mitsutaka Okazaki (2001-2024).

  3DS port			 by h.tomioka(2023-2024)

  C-BIOS			 by BouKiCHi and many people(2002-2018)
  Pseudo Kanji ROM		 by Ａ ｔｏ Ｃ(1997)
  Kaname-Mati as MSX-View(font)  by Ａ ｔｏ Ｃ(1997)