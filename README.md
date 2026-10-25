# Driver for SPL2 and SPLc laser printers from Samsung, Xerox, Dell, Lexmark, and Toshiba

Support for printing to SPL2- and SPLc-based printers. These are most of the cheaper laser printers from Samsung, but also rebranded ones from Xerox, Dell, Lexmark, and Toshiba. This driver is especially for those models not understanding standard languages like PostScript or PCL.

Both monochrome (ML-15xx, ML-16xx, ML-17xx, ML-18xx, ML-2xxx) and color (CLP-5xx, CLP-6xx) models are supported, and also their rebranded equivalents like the Xerox Phaser 6100 work with this driver.

Note that older SPL1-based models (ML-12xx, ML-14xx) do not work. Use these printers with the older "gdi" driver which is built into GhostScript.

See installation instructions in the INSTALL file.

The driver was created by Aurélien Croc (aurelien at ap2c dot org) and contains many contributions from Till Kamppeter (till dot kamppeter at gmail dot com).

### Supported models

| Manufacturer | Model         |
| ------------ | ------------- |
| Samsung | SCX-4100           |
|         | SCX-4200           |
|         | SCX-4216F          |
|         | SCX-4300           |
|         | SCX-4500           |
|         | SCX-4521F          |
|         | SF-565P            |
|         | ML-1510            |
|         | ML-1520            |
|         | ML-1610            |
|         | ML-1710            |
|         | ML-1740            |
|         | ML-1750            |
|         | ML-2571            |
|         | ML-2250            |
|         | ML-2251            |
|         | ML-3471ND          |
|         | ML-3560            |
|         | ML-2150            |
|         | ML-2550            |
|         | ML-1630            |
|         | ML-1640            |
|         | ML-2010            |
|         | ML-2015            |
|         | ML-2240            |
|         | ML-2510            |
|         | ML-1660            |
|         | ML-1910            |
|         | ML-2525            |
|         | ML-2525W           |
|         | ML-2580            |
|         | ML-2580N           |
|         | ML-3050            |
|         | ML-3051            |
|         | ML-3310            |
|         | SCX-3200           |
|         | SCX-3400           |
|         | SCX-4600           |
|         | SCX-4623f          |
|         | SCX-4623fw         |
|         | ML-2160            |
|         | ML-3051ND          |
|         | ML-3310ND          |
|         | SCX-5330N          |
|         | SCX-5530FN         |
|         | ML-1860            |
|         | ML-1670            |
|         | ML-1865            |
|         | ML-1915            |
|         | ML-2165            |
|         | M2020 Series       |
|         | ML-1865W Series    |
|         | M2070 Series       |
|         | M262x 282x Series  |
|         | M267x 287x Series  |
|         | M283x Series       |
|         | CLP-500            |
|         | CLP-550            |
|         | CLP-510            |
|         | CLP-200            |
|         | CLP-600            |
|         | CLX-2170           |
|         | CLP-300            |
|         | CLP-310N           |
|         | CLX-216X           |
|         | CLX-3160           |
|         | CLP-310            |
|         | CLP-315            |
| Xerox   | WorkCentre 3119    |
|         | WorkCentre PE16    |
|         | WorkCentre PE114e  |
|         | Phaser 3115        |
|         | Phaser 3116        |
|         | Phaser 3120        |
|         | Phaser 3121        |
|         | Phaser 3130        |
|         | Phaser 3420        |
|         | Phaser 3425        |
|         | Phaser 5500        |
|         | Phaser 3150        |
|         | Phaser 3160        |
|         | Phaser 3117        |
|         | Phaser 3122        |
|         | Phaser 3124        |
|         | Phaser 3140        |
|         | Phaser 3155        |
|         | Phaser 3020        |
|         | Phaser 3052        |
|         | Phaser 3260        |
|         | WorkCentre 3025    |
|         | WorkCentre 3215    |
|         | WorkCentre 3225    |
|         | Phaser 6100        |
|         | Phaser 6110        |
| Dell    | 1100               |
|         | 1110               |
| HP      | Laser 10x          |
|         | Laser MFP 13x      |
| Toshiba | eSTUDIO180S        |
| Lexmark | X215 MFP           |
