# ArchiverVINAc
Adapted and translated assignment from the programming 2 course.

The objective of this assignment was to implement a program called VINAc, which is an archiver that supports compression of data, similar to programs such as tar and zip. LZ compression was used.

How to use:
"name" will be referred to as the name of the archiver, any name is fine.
"file1" is any file you wish to put in the archiver, same for "file2" and so on.


*To insert files:
./vinac -ip/ic "name".vc "file1" "file2" ...

Ip: inserts plain, uncompressed.
Ic: inserts compressed. If original file is bigger than compressed file, inserts plain.
If the file you are trying to insert has the same name as another file in the archiver, the old file will be removed and the new one added.


*To see the content in the archiver:
./vinac -c "name".vc


*To remove members:
./vinac -r "name".vc "file1" "file2" ...


*To extract members:
./vinac -x "name".vc "file1"

If you do not specify a target file, all files will be removed from the archiver.


*To move members:
./vinac -m "name".vc "file1" "file2"

The archiver will move "file1" to a position immediately AFTER "file2".