
/**********************************FILESYSTEM SIMULATOR*********************************/

/*By
    NIRUPAM BASAK
    SUJAN DAS
    */

/*  command                         action
 *  -------                         -------
 *  ls                          :   print items in current directory
 *  mkdir <name>                :   make a subdirectory
 *  rndir <old_name> <new_name> :   rename directory <old_name> to <new_name>
 *  cd <path>                   :   change directory to path
 *  rmdir <name>                :   remove/delete directory <name>
 *  mvdir <name> <path>         :   move directory <name> to the location <path> (removing original one)
                        (or rename <name> to <path> at same location)
 *  mkfil <name> <size>         :   make file <name> with size <size>
                        (if size is not specified it will be taken as zero)
 *  rnfil <old_name> <new_name> :   rename file <old_name> to <new_name>
 *  rmfil <name>                :   remove/delete file <name>
 *  mvfil <name> <path>         :   move file <name> to the location <path> (removing original one)
                        (or rename <name> to <path> at same location)
 *  exit                        :   terminate the program 
 */
 
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>

#define INPUTSIZE 100
#define PARTITION 1000000
#define BLOCKSIZE 1000
#define BLOCK PARTITION/BLOCKSIZE
#define MAX_LENGTH 20
#define MAX_DATA_BLOCK 100
#define MAX_DIRECTORY 100

/***************************functions to run commands***************************/

bool make_file(char *,char *);
bool run_exit(char *,char *);
bool move_dir(char *,char *);
bool move_file(char *,char *);
bool rename_file(char *,char *);
bool make_file(char *, char *);
bool print_item(char *,char *);
bool rm_file(char *,char *);
bool rm_dir(char *,char *);
bool make_dir(char *,char *);
bool ch_dir(char *,char *);

/******************************additional functions*****************************/

bool add_superblock(char *);
int alloc_block(char *,bool);       //returns index of <arg1> which is of type <arg2>
bool add_dir(char *,char *);        //adds new directory <arg2> with <arg1> as parent
bool init();
void parse(char *,int *,char **);   //parse <arg1> and save it to <arg3> and number of piece to <arg2>
bool add_file(char *,char *,char *);//adds new file <arg2> at <arg1> with data size <arg3>
void print_path();
bool edit_file(char *, char *);     //edits file <arg1> with data size <arg2>
int find_block(char *,char *,bool); //returns index of block named <arg1> of type <arg3> with parent <arg2>
void r_name(char *,char *,bool);    //renames <arg1> to <arg2> of type <arg3>
bool ch_exist(char *,char *,char *,bool);   //checks existance of <arg3> of type <arg4> in directory <arg2> whose parent is <arg1>
void edit_dir(char *,char *,char *,bool,bool);//does <arg5> (true=>add;false=>remove) for <arg3> of type <arg4> in directory <arg2> whose parent is <arg1>
bool del_dir(char *,char *);        //deletes directory <arg1> with <arg2> as parent
bool del_file(char *,char *);       //deletes file <arg1> within directory <arg2>
void parse_s(char *,int *,char **); //parse <arg1> and save it to <arg3> and number of piece to <arg2>
bool dealloc_block(int);            //deallocate block with index <arg>
char *get_parent();                 //returns name of parent directory
void edit_path(char **,int);        //edit current path as <arg1> containing <arg2> many nodes
//all boolean functions return true on success and false on failure

struct super_block
{
    bool Free[BLOCK];               //true denotes free and false denotes allocated
    bool type[BLOCK];               //true denotes folder and false denotes file
    char (*name)[MAX_LENGTH];
};  //keeps track of all free blocks as well as blocks allocated to files or folders along with its name

struct folder
{
    char name[MAX_LENGTH];
    char parent[MAX_LENGTH];
    char (*item)[MAX_LENGTH];
    bool item_type[MAX_LENGTH];
    int item_count;
};  //structure of a folder

struct file
{
    char name[MAX_LENGTH];
    char dir_name[MAX_LENGTH];
    int data_block[MAX_DATA_BLOCK];
    int data_block_count;
    int size;
};  //structure of a file

struct working_dir
{
    char name[MAX_LENGTH];
    char parent[MAX_LENGTH];
};  //denotes working directory

struct working_path
{
    char name[MAX_LENGTH];
    struct working_path *next;
};  //keeps track of current path

char *disk;
struct working_dir working;
struct working_path path;

struct run_cmd
{
    char *cmd;
    bool (*run)(char *name,char *data);
}run_tbl[]={
    {"ls",  print_item},
    {"mkdir",make_dir},
    {"rndir",move_dir},
    {"cd",  ch_dir},
    {"rmdir",rm_dir},
    {"mvdir",move_dir},
    {"mkfil",make_file},
    {"rnfil",move_file},
    {"rmfil",rm_file},
    {"mvfil",move_file},
    {"exit",run_exit},
    {"NONE",NULL}
};  //structure to connect commands to respective functions

/*-----------------------------------------------------------------------------*/
/**********************************Driver code**********************************/

int main()
{
    char input[INPUTSIZE];
    printf("\t\t\t\t**Welcome in the filesystem**\n\t\t\t\t=============================\n");
//initialize disk
    if(!init())
    {
        printf("\tERROR: Disk initialization failed!");
        return 0;
    }
//print current path in shell
    print_path();
    char *cmd,*name,*data;
//get input and run respective command
    while(fgets(input, INPUTSIZE, stdin)!=NULL)
    {
        char *arr[INPUTSIZE];
        int n=0;
//parse the input
        parse(input,&n,arr);
        if(!n)
            continue;               //nothing to run take next command from I/O
        cmd=arr[0];
        name=arr[1];
        data=arr[2];
        bool valid=false;           //true if inserted command is valid
//go through the command list to find appropriate function to run
        for(struct run_cmd *action=run_tbl;action->cmd!="NONE";action++)
//if matched then run corresponding function
            if(!strcmp(cmd,action->cmd))
            {
                valid=true;
//if failed to run then print error message
                if(!((action->run)(name,data)))
                    printf("\tERROR: %s %s: failed\n",cmd,name);
                break;
            }
//if invalid print appropriate message
        if(!valid)
            printf("\t%s: command not found\n",cmd);
        print_path();
    }
    return 0;
}

/*-----------------------------------------------------------------------------*/
/***************************functions to run commands***************************/

//prints list of items in current directory ("ls" command)
bool print_item(char *name,char *empty)
{
    struct folder *dir=malloc(BLOCKSIZE);
//find location of current directory in disk
    int block_index=find_block(working.name,working.parent,true);
//copy current directory block from disk to memory
    memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
//go through the item list and print their name(type)
    for(int i=~-(dir->item_count);~i;i--)
        printf("%s(%s)\t\t",dir->item[i],dir->item_type[i]?"dir":"file");
    printf("\n");
    free(dir);
    return true;
}

//creates new directory ("mkdir" command)
bool make_dir(char *name,char *empty)
{
//null directory or directory name "root" is not allowed
    if(NULL==name||!strcmp(name,"root"))
        return false;
//check wheather there already exists a directory with same name
    if(ch_exist(working.parent,working.name,name,true))
    {
        printf("Directory \"%s\" already exists\n",name);
        return true;
    }
//add the directory to the filesystem
    if(!add_dir(working.name,name))
        return false;
//update current directory adding new directory as an item in it
    edit_dir(working.parent,working.name,name,true,true);
    return true;
}

//moves a directory to specified destination
bool move_dir(char *name,char *loc)
{
    if(!ch_exist(working.parent,working.name,name,true))
    {
        printf("\tNo such directory\n");
        return true;
    }
    char *dest[INPUTSIZE];
    int n=0;
    parse_s(loc,&n,dest);
    if(!n)
        return false;
//either destination is "root" or rename name to dest[0]
    if(!~-n)
    {
//rename the directory
        if(strcmp(dest[0],"root"))
        {
            if(ch_exist(working.parent,working.name,dest[0],true))
            {
                printf("\tdirectory \"%s\" already exists\n",dest[0]);
                return true;
            }
            r_name(name,dest[0],true);
            return true;
        }
        if(!strcmp(working.name,"root"))
            return true;
        if(ch_exist("","root",name,true))
        {
            printf("\tDirectory already exists.\n");
            return true;
        }
        edit_dir(working.parent,working.name,name,true,false);
        struct folder *dir=malloc(BLOCKSIZE);
        int block_index=find_block(name,working.name,true);
        memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
        strcpy(dir->parent,"root");
        memcpy(disk+block_index*BLOCKSIZE,dir,BLOCKSIZE);
        free(dir);
        edit_dir("","root",name,true,true);
        return true;
    }
//destination is not "root"
//check validity of the path
    if(strcmp(dest[0],"root")||!ch_exist("",dest[0],dest[1],true))
    {
        printf("\tInvalid path\n");
        return false;
    }
    if(!(strcmp(working.name,dest[0])&&(strcmp(working.name,dest[1])||strcmp(working.parent,dest[0]))))
    {
        printf("\tInvalid move\n");
        return true;
    }
    for(int i=2;i<n;i++)
    {
        if(!ch_exist(dest[~-~-i],dest[~-i],dest[i],true))
        {
            printf("\tInvalid path\n");
            return false;
        }
        if(!(strcmp(working.name,dest[i])||strcmp(working.parent,dest[~-i])))
        {
            printf("\tInvalid move\n");
            return true;
        }
    }
    edit_dir(working.parent,working.name,name,true,false);
    struct folder *dir=malloc(BLOCKSIZE);
    int block_index=find_block(name,working.name,true);
    memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
    strcpy(dir->parent,dest[~-n]);
    memcpy(disk+block_index*BLOCKSIZE,dir,BLOCKSIZE);
    free(dir);
    edit_dir(dest[~-~-n],dest[~-n],name,true,true);
    return true;
}

//removes a subdirectory from current directory ("rmdir" command)
bool rm_dir(char *name,char *empty)
{
//check validity/existance of subdirectory
    if(!(strcmp(name,"")&&strcmp(name,".")&&strcmp(name,"..")&&ch_exist(working.parent,working.name,name,true)))
    {
        printf("\tNo such directory\n");
        return true;
    }
//update current directory item list
    edit_dir(working.parent,working.name,name,true,false);
//remove the directory from filesystem
    if(!del_dir(name,working.name))
    {
//if failed the restore current directory item list
        edit_dir(working.parent,working.name,name,true,true);
        return false;
    }
    return true;
}

//change current working directory ("chdir" command)
bool ch_dir(char *input,char *empty)
{
    char *name[INPUTSIZE];
    int n=0;
//parse the destination
    parse_s(input,&n,name);
//no destination specified
    if(!n)
        return false;
//destination contains one name so it must be either itself or "root" or parent or subdirectory
    if(!~-n)
    {
//destination is current directory itself
        if(!strcmp(name[0],"."))
            return true;
//destination is parent directory
        if(!strcmp(name[0],".."))
        {
//if current directory is "root" then there is no parent
            if(!strcmp(working.name,"root"))
                return true;
            char *parent=get_parent();
            strcpy(working.name,working.parent);
            strcpy(working.parent,parent);
//update current path
            edit_path(name,n);
            return true;
        }
//destination is "root"
        if(!strcmp(name[0],"root"))
        {
            strcpy(working.parent,"");
            strcpy(working.name,"root");
            edit_path(name,n);
            return true;
        }
//destination is a subdirectory; check existance of that subdirectory
        if(!ch_exist(working.parent,working.name,name[0],true))
        {
            printf("\tNo such directory\n");
            return true;
        }
        strcpy(working.parent,working.name);
        strcpy(working.name,name[0]);
        edit_path(name,n);
        return true;
    }
//whole path of destination is specified; check validity of the path
    if(strcmp(name[0],"root")||!ch_exist("",name[0],name[1],true))
    {
        printf("\tInvalid path\n");
        return true;
    }
    for(int i=2;i<n;i++)
        if(!ch_exist(name[~-~-i],name[~-i],name[i],true))
        {
            printf("\tInvalid path\n");
            return true;
        }
    strcpy(working.parent,name[~-~-n]);
    strcpy(working.name,name[~-n]);
    edit_path(name,n);
    return true;
}

//creates new file ("mkfil" command)
bool make_file(char *name, char *data)
{
//no name specified or file named "root" is not allowed
    if(NULL==name||!strcmp(name,"root"))
        return false;
//check wheather there already exists a file with same name
    if(ch_exist(working.parent,working.name,name,false))
    {
        char a;
        printf("File already exists. Do you want to EDIT it (if yes, type y/Y; otherwise type any other key)?\t");
        scanf("%c",&a);
        if('y'==a||'Y'==a)
            return edit_file(name,data);
        return true;
    }
//add the file to the filesystem
    if(!add_file(working.name,name,data))
        return false;
    edit_dir(working.parent,working.name,name,false,true);
    return true;
}

//moves a file to specified destination
bool move_file(char *name,char *loc)
{
    if(!ch_exist(working.parent,working.name,name,false))
    {
        printf("\tNo such file\n");
        return true;
    }
    char *dest[INPUTSIZE];
    int n=0;
    parse_s(loc,&n,dest);
    if(!n)
        return false;
//either destination is "root" or rename name to dest[0]
    if(!~-n)
    {
//rename the file
        if(strcmp(dest[0],"root"))
        {
            if(ch_exist(working.parent,working.name,dest[0],false))
            {
                printf("\tfile \"%s\" already exists\n",dest[0]);
                return true;
            }
            r_name(name,dest[0],false);
            return true;
        }
        if(!strcmp(working.name,"root"))
            return true;
        if(ch_exist("","root",name,false))
        {
            printf("\tFile already exists.\n");
            return true;
        }
        edit_dir(working.parent,working.name,name,false,false);
        struct file *fp=malloc(BLOCKSIZE);
        int block_index=find_block(name,working.name,false);
        memcpy(fp,disk+block_index*BLOCKSIZE,BLOCKSIZE);
        strcpy(fp->dir_name,"root");
        memcpy(disk+block_index*BLOCKSIZE,fp,BLOCKSIZE);
        free(fp);
        edit_dir("","root",name,false,true);
        return true;
    }
//destination is not "root"
    if(strcmp(dest[0],"root")||!ch_exist("",dest[0],dest[1],true))
    {
        printf("\tInvalid path\n");
        return false;
    }
    for(int i=2;i<n;i++)
        if(!ch_exist(dest[~-~-i],dest[~-i],dest[i],true))
        {
            printf("\tInvalid path\n");
            return false;
        }
    edit_dir(working.parent,working.name,name,false,false);
    struct file *fp=malloc(BLOCKSIZE);
    int block_index=find_block(name,working.name,false);
    memcpy(fp,disk+block_index*BLOCKSIZE,BLOCKSIZE);
    strcpy(fp->dir_name,dest[~-n]);
    memcpy(disk+block_index*BLOCKSIZE,fp,BLOCKSIZE);
    free(fp);
    edit_dir(dest[~-~-n],dest[~-n],name,false,true);
    return true;
}

//removes a file from current directory ("rmfil" command)
bool rm_file(char *name,char *empty)
{
//check validity/existance of the file
    if(!(strcmp(name,"")&&ch_exist(working.parent,working.name,name,false)))
    {
        printf("\tNo such file\n");
        return true;
    }
    edit_dir(working.parent,working.name,name,false,false);
//remove the file from filesystem
    if(!del_file(name,working.name))
    {
        edit_dir(working.parent,working.name,name,false,true);
        return false;
    }
    return true;
}

//exit from the program
bool run_exit(char *name,char *empty)
{
    exit(0);
    return true;
}

/*-----------------------------------------------------------------------------*/
/******************************additional functions*****************************/

//parse the input with whitespace as delimiter
void parse(char *input,int *num,char *parsed[])
{
    char *del;                              //address of delimeter in input
    char sp[]=" \t\n";                      //array of delimiters
    while(true)
    {
        input+=strspn(input,sp);            //skip initial whitespace
//if there is no more whitespace then break
        if(NULL==(del=strpbrk(input,sp)))   //get next delimiter
            break;
        parsed[(*num)++]=input;             //add to the parsed array
        *del='\0';                          //specify end of string
        input=del+1;
    }
//if number of pieces is less than three then add null strings
    if(!(*num))
        return;
    while(3>(*num))
        parsed[(*num)++]="";
}

//parse string with '/' as delimiter
void parse_s(char *input,int *num,char *parsed[])
{
//if input is null then nothing to parse
    if(NULL==input)
        return;
    char *del;
    char bs[]="\\";
    while(true)
    {
        input+=strspn(input,bs);
        if(NULL==(del=strpbrk(input,bs)))
            break;
        parsed[(*num)++]=input;
        *del='\0';
        input=del+1;
    }
//add the last substring
    if(!strcmp(input,""))
        return;
    parsed[(*num)++]=input;
}

//goes through the linked list working_path and prints current path
void print_path()
{
    struct working_path *p=&path;
    while(NULL!=p)
    {
        printf("\\%s",p->name);
        p=p->next;
    }
    printf("> ");
}

//edit working_path
void edit_path(char *name[],int n)
{
    struct working_path *p=&path;
    if(!~-n)
    {
//parent directory will be next working directory
        if(!(strcmp(name[0],"..")))
        {
            while(NULL!=((p->next)->next))
                p=p->next;
            p->next=NULL;
            return;
        }
//root directory will be next working directory
        if(!strcmp(name[0],"root"))
        {
            p->next=NULL;
            return;
        }
//subdirectory will be next working directory
        while(NULL!=(p->next))
            p=p->next;
        struct working_path *new=malloc(sizeof(struct working_path));
        strcpy(new->name,name[0]);
        new->next=NULL;
        p->next=new;
        return;
    }
//working_path will be updated according to the specified path
    for(int i=1;i<n;i++)
    {
        struct working_path *new=malloc(sizeof(struct working_path));
        strcpy(new->name,name[i]);
        new->next=NULL;
        p->next=new;
        p=p->next;
    }
    return;
}

//parent of the parent of current working directory can be found from working_path
char *get_parent()
{
    struct working_path *p=&path;
    char *parent=malloc(MAX_LENGTH*sizeof(char));
    if(NULL==(p->next)->next)
    {
        strcpy(parent,"");
        return parent;
    }
    while(NULL!=(((p->next)->next)->next))
        p=p->next;
    strcpy(parent,p->name);
    return parent;
}

//initialize the disk
bool init()
{
    disk=malloc(PARTITION);
//create superblock
    add_superblock("superblock");
//add root directory
    if(!add_dir("","root"))
        return false;
//update working directory
    strcpy(working.name,"root");
    strcpy(working.parent,"");
//update working path
    strcpy(path.name,"root");
    path.next=NULL;
    return true;
}

//creates superblock
bool add_superblock(char *name)
{
    struct super_block *sblock=malloc(BLOCKSIZE<<2);
    sblock->name=malloc(BLOCK*sizeof(*name));
    int sblock_size=(int)(sizeof(struct super_block)/BLOCKSIZE)+1;
    for(int i=~-BLOCK;~i;i--)
    {
//space taken by superblock
        if(i<sblock_size)
        {
            sblock->Free[i]=false;
            strcpy(sblock->name[i],name);
        }
//space for other files and folders
        else
            sblock->Free[i]=true;
        sblock->type[i]=false;
    }
    memcpy(disk,sblock,(BLOCKSIZE*sblock_size));
    free(sblock);
    return true;
}

//allocates blocks for files or folders
int alloc_block(char *name,bool type)
{
    struct super_block *sblock=malloc(BLOCKSIZE<<2);
    sblock->name=malloc(BLOCK*sizeof(name));
    memcpy(sblock,disk,(BLOCKSIZE<<2));
    for(int i=0;i<BLOCK;i++)
//find free block
        if(sblock->Free[i])
        {
//allocate the free block to new file or folder and update superblock accordingly
            sblock->Free[i]=false;
            sblock->type[i]=type;
            strcpy(sblock->name[i],name);
            memcpy(disk,sblock,(BLOCKSIZE<<2));
            free(sblock);
            return i;
        }
    free(sblock);
    return -1;
}

//finds index of specific block with specified type and parent
int find_block(char *name,char *parent,bool type)
{
    struct super_block *sblock=malloc(BLOCKSIZE<<2);
    memcpy(sblock,disk,(BLOCKSIZE<<2));
    for(int i=0;i<BLOCK;i++)
//find matching name
        if(type==sblock->type[i]&&!strcmp(sblock->name[i],name))
        {
//checks type
            if(true==type)
            {
                struct folder *dir=malloc(BLOCKSIZE);
                memcpy(dir,disk+i*BLOCKSIZE,BLOCKSIZE);
//checks parent
                if(!strcmp(dir->parent,parent))
                {
                    free(dir);
                    free(sblock);
                    return i;
                }
            }
            else
            {
                struct file *fp=malloc(BLOCKSIZE);
                memcpy(fp,disk+i*BLOCKSIZE,BLOCKSIZE);
//chacks parent
                if(!strcmp(fp->dir_name,parent))
                {
                    free(fp);
                    free(sblock);
                    return i;
                }
            }
        }
    free(sblock);
    return -1;
}

//deallocate block by editing super block
bool dealloc_block(int index)
{
    struct super_block *sblock=malloc(BLOCKSIZE<<2);
    memcpy(sblock,disk,BLOCKSIZE<<2);
    sblock->Free[index]=true;
    strcpy(sblock->name[index],"");
    memcpy(disk,sblock,BLOCKSIZE<<2);
    free(sblock);
}

//adds new directory to the filesystem
bool add_dir(char *parent,char *name)
{
//create the folder
    struct folder *dir=malloc(BLOCKSIZE);
    strcpy(dir->name,name);
    strcpy(dir->parent,parent);
    dir->item=malloc(MAX_DIRECTORY*sizeof(dir->item));
    dir->item_count=0;
    int i;
//allocate block for the folder
    if(-1==(i=alloc_block(name,true)))
    {
        free(dir);
        return false;
    }
    memcpy(disk+i*BLOCKSIZE,dir,BLOCKSIZE);
    free(dir);
    return true;
}

//adds file to the filesystem
bool add_file(char *dir,char *name,char *data)
{
//create the file
    struct file *fp=malloc(BLOCKSIZE);
    strcpy(fp->name,name);
    strcpy(fp->dir_name,dir);
    fp->data_block_count=0;
    fp->size=(NULL==data?0:atoi(data));
    int i,j,k;
//allocate block for the file
    if(-1==(k=alloc_block(name,false)))
    {
        free(fp);
        return false;
    }
//allocate block for data
    char sub[MAX_LENGTH];
    for(i=(fp->size)/BLOCKSIZE;~i;i--)
    {
        sprintf(sub,"%s[%d]",name,i);
        if(-1==(j=alloc_block(sub,false)))
        {
//if allocation of block failed at any point of time then deallocate all the blocks allocated to this file
            for(++i;i<(fp->size)/BLOCKSIZE;i++)
                dealloc_block(fp->data_block[i]);
            dealloc_block(k);
            free(fp);
            return false;
        }
        fp->data_block[i]=j;
        fp->data_block_count++;
    }
    memcpy(disk+k*BLOCKSIZE,fp,BLOCKSIZE);
    free(fp);
    return true;
}

//add or remove item from item list
void edit_dir(char *cur_par,char *cur_dir,char *name,bool type,bool add)
{
//add=true means add the item at the end of item list
    if(add)
    {
        struct folder *dir=malloc(BLOCKSIZE);
        int block_index=find_block(cur_dir,cur_par,true);
        memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
        strcpy(dir->item[dir->item_count],name);
        dir->item_type[dir->item_count]=type;
        dir->item_count++;
        memcpy(disk+block_index*BLOCKSIZE,dir,BLOCKSIZE);
        free(dir);
    }
//add=false means remove the item
    else
    {
        struct folder *dir=malloc(BLOCKSIZE);
        int block_index=find_block(cur_dir,cur_par,true);
        memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
        for(int i=~-(dir->item_count);~i;i--)
        {
//check for type
            if(type!=dir->item_type[i])
                continue;
//check for name
            if(!strcmp(name,dir->item[i]))
            {
//copy last item at the position of deleting item and decrease item count
                strcpy(dir->item[i],dir->item[--(dir->item_count)]);
                dir->item_type[i]=dir->item_type[dir->item_count];
            }
        }
        memcpy(disk+block_index*BLOCKSIZE,dir,BLOCKSIZE);
        free(dir);
    }
}

//edits a file size
bool edit_file(char *name, char *data)
{
    if(!ch_exist(working.parent,working.name,name,false))
    {
        printf("\tNo such file\n");
        return true;
    }
    struct file *fp=malloc(BLOCKSIZE);
    int block_index=find_block(name,working.name,false);
    memcpy(fp,disk+block_index*BLOCKSIZE,BLOCKSIZE);
//if size is same then nothing to do
    if(atoi(data)==fp->size)
    {
        free(fp);
        return true;
    }
//temporarily rename the file as "root"
    r_name(name,"root",false);
//create a new file
    if(!make_file(name,data))
    {
//if failed restore the name of previous file
        r_name("root",name,false);
        free(fp);
        return false;
    }
//if succeed then delete the temporary file "root"
    rm_file("root","");
    free(fp);
    return true;
}

//renames a file
void r_name(char *old_name,char *new_name,bool type)
{
    struct super_block *sblock=malloc(BLOCKSIZE<<2);
    memcpy(sblock,disk,(BLOCKSIZE<<2));
    for(int i=0;i<BLOCK;i++)
//find block index for the file
        if(type==sblock->type[i]&&!strcmp(sblock->name[i],old_name))
        {
            if(type)
            {
                struct folder *dir=malloc(BLOCKSIZE);
                memcpy(dir,disk+i*BLOCKSIZE,BLOCKSIZE);
//check parent
                if(!strcmp(dir->parent,working.name))
                {
//update superblock
                    strcpy(sblock->name[i],new_name);
//update file
                    strcpy(dir->name,new_name);
                    memcpy(disk+i*BLOCKSIZE,dir,BLOCKSIZE);
                    free(dir);
                    break;
                }
            }
            struct file *fp=malloc(BLOCKSIZE);
            memcpy(fp,disk+i*BLOCKSIZE,BLOCKSIZE);
//check parent
            if(!strcmp(fp->dir_name,working.name))
            {
//update superblock
                strcpy(sblock->name[i],new_name);
//update file
                strcpy(fp->name,new_name);
                memcpy(disk+i*BLOCKSIZE,fp,BLOCKSIZE);
                free(fp);
                break;
            }
        }
    for(int i=0;i<BLOCK;i++)
//find block index for current directory
        if(sblock->type[i]&&!strcmp(sblock->name[i],working.name))
        {
            struct folder *dir=malloc(BLOCKSIZE);
            memcpy(dir,disk+i*BLOCKSIZE,BLOCKSIZE);
//check parent of the directory
            if(!strcmp(dir->parent,working.parent))
            {
//update item list
                for(int j=~-(dir->item_count);~j;j--)
                {
                    if(type!=dir->item_type[j])
                        continue;
                    if(!strcmp(old_name,dir->item[j]))
                    {
                        strcpy(dir->item[j],new_name);
                        memcpy(disk+i*BLOCKSIZE,dir,BLOCKSIZE);
                        free(dir);
                        break;
                    }
                }
            }
        }
    memcpy(disk,sblock,BLOCKSIZE<<2);
    free(sblock);
}

//checks existance of a file or folder
bool ch_exist(char *cur_par,char *cur_name,char *name,bool type)
{
    struct folder *dir=malloc(BLOCKSIZE);
    int block_index=find_block(cur_name,cur_par,true);
    memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
    for(int i=~-(dir->item_count);~i;i--)
    {
//checks for matching type
        if(type!=dir->item_type[i])
            continue;
//checks for matching name
        if(!(strcmp(dir->item[i],name)))
        {
            free(dir);
            return true;
        }
    }
    free(dir);
    return false;
}

//delete directory from the filesystem
bool del_dir(char *name,char *parent)
{
    struct folder *dir=malloc(BLOCKSIZE);
    int block_index=find_block(name,parent,true);
    memcpy(dir,disk+block_index*BLOCKSIZE,BLOCKSIZE);
//delete subitems recursively
    for(int i=~-(dir->item_count);~i;i--)
        if(dir->item_type[i])
            if(!del_dir(dir->item[i],name))
            {
                free(dir);
                return false;
            }
            else
                continue;
        else
            if(!del_file(dir->item[i],name))
            {
                free(dir);
                return false;
            }
            else
                continue;
//deallocate the block
    if(!dealloc_block(block_index))
    {
        free(dir);
        return false;
    }
    free(dir);
    return true;
}

//deletes file from the filesystem
bool del_file(char *name,char *dir)
{
    struct file *fp=malloc(BLOCKSIZE);
    int block_index=find_block(name,dir,false);
    memcpy(fp,disk+block_index*BLOCKSIZE,BLOCKSIZE);
//deallocate all data blocks
    for(int i=~-(fp->data_block_count);~i;i--)
        if(!dealloc_block(fp->data_block[i]))
        {
            free(fp);
            return false;
        }
//deallocate block for file
    if(!dealloc_block(block_index))
    {
        free(fp);
        return false;
    }
    free(fp);
    return true;
}

