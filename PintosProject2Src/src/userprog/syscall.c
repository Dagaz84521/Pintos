#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "pagedir.h"
#include <threads/vaddr.h>
#include <filesys/filesys.h>
#include "devices/shutdown.h"


static void (*syscalls[21])(struct intr_frame *);

static void syscall_handler (struct intr_frame *);
void sys_halt(struct intr_frame * f);
void sys_exit(struct intr_frame * f);
void sys_exec(struct intr_frame * f);
void sys_wait(struct intr_frame * f);
void sys_create(struct intr_frame * f);
void sys_remove(struct intr_frame * f);
void sys_open(struct intr_frame * f);
void sys_filesize(struct intr_frame * f);
void sys_read(struct intr_frame * f);
void sys_write(struct intr_frame * f);
void sys_seek(struct intr_frame * f);
void sys_tell(struct intr_frame * f);
void sys_close(struct intr_frame * f);
struct thread_file *find_file_id (int file_id);
void exit_special();
void* is_valid_ptr(const void* vaddr);
static int get_user(const uint8_t *uaddr);
bool is_valid_pointer (void* esp,uint8_t argc);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  syscalls[SYS_HALT] = &sys_halt;
  syscalls[SYS_EXIT] = &sys_exit;
  syscalls[SYS_EXEC] = &sys_exec;
  syscalls[SYS_WAIT] = &sys_wait;
  syscalls[SYS_CREATE] = &sys_create;
  syscalls[SYS_REMOVE] = &sys_remove;
  syscalls[SYS_OPEN] = &sys_open;
  syscalls[SYS_FILESIZE] = &sys_filesize;
  syscalls[SYS_READ] = &sys_read;
  syscalls[SYS_WRITE] = &sys_write;
  syscalls[SYS_SEEK] = &sys_seek;
  syscalls[SYS_TELL] = &sys_tell;
  syscalls[SYS_CLOSE] = &sys_close;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp;
  is_valid_ptr(p + 1);
  int type = * (int *)f->esp;	
  if(type <= 0 || type >= 21){
    exit_special ();		//类型错误，退出
  }
  // printf("syscall:%d\n",type);
  syscalls[type](f);
  // printf ("system call!\n");
  // thread_exit ();
}

void sys_halt(struct intr_frame *f){
	shutdown_power_off();
}

void sys_exit(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 1);
  *user_ptr++;

  thread_current()->exit_status = *user_ptr;	
  thread_exit ();
}

void sys_exec(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr(user_ptr+1);
  is_valid_ptr (*(user_ptr+1));
  *user_ptr++;
  f->eax=process_execute((char*)*user_ptr);
}

void sys_wait(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 1);
  *user_ptr++;
  f->eax = process_wait(*user_ptr);
}

void sys_create(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 5);
  is_valid_ptr (*(user_ptr + 4));
  *user_ptr++;
  acquire_lock_f ();
  f->eax = filesys_create ((const char *)*user_ptr, *(user_ptr+1));
  release_lock_f ();
}

void sys_remove(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 1);
  is_valid_ptr (*(user_ptr + 1));
  *user_ptr++;
  acquire_lock_f ();
  f->eax = filesys_remove ((const char *)*user_ptr);
  release_lock_f ();
}

void sys_open(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 1);
  is_valid_ptr (*(user_ptr + 1));
  *user_ptr++;
  acquire_lock_f ();
  struct file * file_opened = filesys_open((const char *)*user_ptr);
  release_lock_f ();
  struct thread * t = thread_current();
  if (file_opened)
  {
    struct thread_file *thread_file_temp = malloc(sizeof(struct thread_file));
    thread_file_temp->fd = t->file_fd++;
    thread_file_temp->file = file_opened;
    list_push_back (&t->files, &thread_file_temp->file_elem);//维护files列表
    f->eax = thread_file_temp->fd;
  } 
  else// the file could not be opened
  {
    f->eax = -1;
  }
}

void sys_filesize(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 1);
  *user_ptr++;//fd
  struct thread_file * thread_file_temp = find_file_id (*user_ptr);
  if (thread_file_temp)
  {
    acquire_lock_f ();
    f->eax = file_length (thread_file_temp->file);//return the size in bytes
    release_lock_f ();
  } 
  else
  {
    f->eax = -1;
  }
}

void sys_read(struct intr_frame * f){
   uint32_t *user_ptr = f->esp;
  /* PASS the test bad read */
  *user_ptr++;
  /*获取参数*/
  int fd = *user_ptr;
  uint8_t * buffer = (uint8_t*)*(user_ptr+1);
  off_t size = *(user_ptr+2);
  /*判断文件访问有效*/
  if (!is_valid_pointer (buffer, 1) || !is_valid_pointer (buffer + size,1)){
    exit_special ();
  }
  /*是否为标准输入流*/
  if (fd == 0) //stdin
  {
   //标准输入流就用input_getc();
    for (int i = 0; i < size; i++)
      buffer[i] = input_getc();
    f->eax = size;
  }
  else
  {
  //找到文件
    struct thread_file * thread_file_temp = find_file_id (*user_ptr);
    if (thread_file_temp)
    {
        //读取文件
      acquire_lock_f ();
      f->eax = file_read (thread_file_temp->file, buffer, size);
      release_lock_f ();
    } 
    else//can't read
    {
      f->eax = -1;
    }
  }
}

void sys_write(struct intr_frame * f){
  uint32_t *user_ptr=f->esp;
  is_valid_ptr(user_ptr+7);
  is_valid_ptr(*(user_ptr+6));
  *user_ptr++;
  int fd=*user_ptr;
  const char * buffer = (const char *)*(user_ptr+1);
  off_t file_size = *(user_ptr+2);
  if(fd==1){
    putbuf(buffer,file_size);
    f->eax=file_size;
  }else{
    struct thread_file* thread_file_temp = find_file_id(*user_ptr);
    if(thread_file_temp){
      acquire_lock_f();
      f->eax=file_write(thread_file_temp->file,buffer,file_size);
      release_lock_f();
    }
    else
      f->eax=0;
  }
}

struct thread_file *find_file_id (int file_id)
{
  struct list_elem *e;
  struct thread_file * thread_file_temp = NULL;
  struct list *files = &thread_current ()->files;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
    thread_file_temp = list_entry (e, struct thread_file, file_elem);
    if (file_id == thread_file_temp->fd)
      return thread_file_temp;
  }
  return false;
}

void sys_seek(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 5);
  *user_ptr++;//fd
  struct thread_file *file_temp = find_file_id (*user_ptr);
  if (file_temp)
  {
    acquire_lock_f ();
    file_seek (file_temp->file, *(user_ptr+1));
    release_lock_f ();
  }
}

void sys_tell(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr(user_ptr + 1);
  *user_ptr++;
  struct thread_file *thread_file_temp = find_file_id (*user_ptr);
  if (thread_file_temp)
  {
    acquire_lock_f ();
    f->eax = file_tell (thread_file_temp->file);
    release_lock_f ();
  }else{
    f->eax = -1;
  }
}

void sys_close(struct intr_frame * f){
  uint32_t *user_ptr = f->esp;
  is_valid_ptr (user_ptr + 1);
  *user_ptr++;
  struct thread_file * opened_file = find_file_id (*user_ptr);
  if (opened_file)
  {
    acquire_lock_f ();
    file_close (opened_file->file);
    release_lock_f ();
    /* Remove the opened file from the list */
    list_remove (&opened_file->file_elem);
    /* Free opened files */
    free (opened_file);
  }
}








void exit_special(){
	thread_current()->exit_status=-1;
	thread_exit();
}

void * is_valid_ptr(const void* vaddr)
{
    //先判断是否是用户的地址，如果是非用户地址，那么就是非法访问。
	if(!is_user_vaddr(vaddr))
	{
		exit_special();
	}
    //判断地址是否正确指向页
	void *ptr = pagedir_get_page(thread_current()->pagedir,vaddr);
	if(!ptr)
	{
		exit_special();
	}
    //判断页内容是否正确
	uint8_t *check_byteptr = (uint8_t*) vaddr;
	for(uint8_t i=0;i<4;i++)
	{
		if(get_user(check_byteptr+i)==-1)
		{
			exit_special();
		}
	}
}

static int get_user(const uint8_t *uaddr)
{
	int res;
	asm ("movl $1f, %0; movzbl %1, %0; 1:" : "=&a" (res) : "m" (*uaddr));
	return res;
}

bool 
is_valid_pointer (void* esp,uint8_t argc){
  for (uint8_t i = 0; i < argc; ++i)
  {
    if((!is_user_vaddr (esp)) || 
      (pagedir_get_page (thread_current()->pagedir, esp)==NULL)){
      return false;
    }
  }
  return true;
}
