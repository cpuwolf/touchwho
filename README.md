# touchwho
 touchwho is a program that monitor subfolder files in a folder

 touchwho helps you follow who(file) has been touched

 touchwho 2.0 supports Linux only.

 what will touchwho do?

 1. touchwho walks through all the files in folder
 2. touchwho counts number of files
 3. touchwho add inotify watch on each files
 4. ...wait..., other Linux processes can open the files that are being watched
 5. user use Ctrl+C to exit waiting
 6. touchwho writes down touched file list to a file.
