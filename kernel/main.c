
int main(){

    asm("movl $0,%eax");
    asm("movl $0,%ebx");
    asm("movl $0,%ecx");
    asm("movl $0,%edx");
    while(1);
    return 0;
}