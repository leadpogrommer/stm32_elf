char data_string[] = "This string will have numbers:             \n";
char bss_string[40];

int prog_main(){
    printf("Hello, world: this should be in rodata\n ");
    for(int i = 0; i < 10; i++){
        data_string[i+30] = '0'+i;
    }
    printf(data_string);
    for(int i = 0; i < 'z'-'a'; i++){
        bss_string[i] = 'a'+i;
    }
    bss_string['z']='\n';
    bss_string['z'+1]=0;
    printf(bss_string);

    return 3;
}