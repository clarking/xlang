extern void printf(char*, int);
extern void scanf(char*, int);

void printfstr(char* str)
{
  printf(str, 0);
}

global int main()
{
  int num, first, second, next, c;
  first = 0;
  second = 1;
 
  printfstr("Enter the number of terms: ");
  scanf("%d", &num);
  
  for(c = 0; c < num; c++){
    if(c <= 1){
      next = c;
    }else{
      next = first + second;
      first = second;
      second = next;
    }
    printf("%d\n", next);
  }
}
