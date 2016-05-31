


bool test(){
  int x = 1;
  int y = -(++x);

  bool checkpoint = y == -2 && x == 2;

  int z = +(x++);
  checkpoint = checkpoint && z == 2 && x == 3;


  int array[3];
  array[0] = 1;
  array[1] = 2;
  array[2] = 3;

  int i = 0; 
  int a = ++array[++i];

  checkpoint = checkpoint && i == 1 && a == 3 && array[1] == 3;

  
  
  array[0] = 1;
  array[1] = 2;
  array[2] = 3;
  i = 1;
  a = --array[--i];
  checkpoint = checkpoint && i == 0 && a == 0 && array[0] == 0;


  return checkpoint;
}

