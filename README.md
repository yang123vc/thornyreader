# ThornyReader

## About

ThornyReader is a program for rendering EPUB without DRM,
PDF without DRM, DOC, RTF, TXT, DJVU, FB2 and MOBI docs.

## Usage

ThornyReader operates via CLI. Basic principle:
- create two FIFO files
- choose desired binary:
  - crengine for EPUB, DOC, RTF, TXT, MOBI and CHM
  - mupdf for PDF, XPS and OXPS
  - djvu for DJVU
- start chosen binary and pass two created FIFO files as arguments
- write requests to first FIFO and read responses from second FIFO

## Build

Build is performed by Android Studio. Build should create three binaries:
crengine, mupdf and djvu. Application.mk sample:
```
APP_MODULES             := crengine mupdf djvu
APP_PLATFORM            := android-16
APP_STL                 := c++_static
APP_ABI                 := armeabi-v7a arm64-v8a x86 x86_64
```

## Third-party

ThornyReader uses number of other projects, mainly CoolReader, MuPDF
and DjVuLibre. You can find all original copyright notices for used projects
in the corresponding source directories.

## Contacts

If you have any questions, feel free to contact: thornyreader@gmail.com

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.