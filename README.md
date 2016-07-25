# touchwho
 touchwho is a program that monitor subfolder files in a folder

 touchwho 1.0 supports Linux only.

 what will touchwho do?

 1. touchwho goes through all the files in folder
 2. touchwho counts the number of files
 3. touchwho add inotify watch on each files
 4. ...wait...at the same time, another processes can open the files that is being watched
 5. user use Ctrl+C to exit waiting
 6. touchwho writes down touched file list to a file.
