int main(int argc, char** argv)
{
  int pid = fork();
  if ( pid ) 
    /* I'm the father, do something.. */
  else
    /* I'm the son */
    exec("/path/to/pogram", args[]);

  /*  This part is never reached by the son.
   *  Its code segment is erased by the exec() call
   */
  return EXIT_SUCCESS;
}  
