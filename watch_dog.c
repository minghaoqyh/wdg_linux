#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <string.h>             /* bzero */
#include <stdlib.h>             /*exit   */
#include <sys/times.h>          /* times*/
#include <sys/types.h>          /* pid_t*/
#include <termios.h>            /*termios, tcgetattr(), tcsetattr()*/
#include <unistd.h>
#include <sys/ioctl.h>          /* ioctl*/
#include <errno.h>
// #include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include "uart.h"

#if 1
#define DBG(x...) printf(x)
#else
#define DBG(x...) do { } while(0)
#endif

#define uart_debug DBG

#define SAVE_LOG
const char * log_file_path = "/var/log/uart_log.txt";

#define FEED_TIME 1 //second
#define DEFAULT_BAUDRATE 9600

struct send_data {
    unsigned char timeout;
    unsigned char feed_value;
    unsigned char reset_value;     
}; 


int feed_dog(int fd, unsigned char feed_val);
int set_timeout(int fd, unsigned char timeout);
// int auto_scan(int *fd_dev);
int auto_scan(int *fd, unsigned char value);
char *get_time(void);
void write_log(const char* ms, ... );
int send_to_uart(int fd, unsigned char value);


int main(int argc, char **argv)
{
    int ret = 0;
    int fd_dev[20];
    int device_num = 0;
    int i = 0;
    struct send_data send_uart_data = {
        .timeout = 18,//1-127
        .feed_value = 128,//128-254
        .reset_value = 255,//255
		//.disable_24r = 0xf3 ,//关闭24小时硬重启
    };

    // if(argc < 2){
    //  printf("Usage: %s device buad.\n", argv[0]);
    //     printf("for example: %s /dev/ttyHSL0 115200", argv[0]);
    //     ret = -1;
    //  goto exit;
    // }

    uart_debug("[ %s ] : *******system start*********.\n", get_time());
    write_log("*******system start*********.\n");

    device_num = auto_scan(fd_dev, send_uart_data.feed_value);
    for(i = 0; i < device_num; i++){
        if(send_to_uart(fd_dev[i], send_uart_data.timeout) == 0){//det timeout
            uart_debug("set timeout successful.\n");
        }
    }
    //uart_debug("%s : device_num = %d.\n", get_time(), device_num);
    //write_log("device_num = %d.\n", device_num);
    while(1)
    {
        if(device_num == 0){
            uart_debug("there is not uart device.\n");
            device_num = auto_scan(fd_dev, send_uart_data.feed_value);
            //write_log("there is not uart device.\n");
        }else{
            for(i = 0; i < device_num; i++){
                // if(feed_dog(fd_dev[i], FEED_VALUE) < 0){
                if(send_to_uart(fd_dev[i], send_uart_data.feed_value) < 0){
                    static fail_time = 0;
                    fail_time++;
                    if(fail_time >= 2){
                        device_num = 0;//start auto_scan
                    }
                    uart_debug("feed_dog error.\n");
                }
            }
        }
        sleep(FEED_TIME);
    }
    uart_debug("main end!\n");
    write_log("+++++++++system end! ++++++++\n\n");

exit:
    for(i = 0; i < device_num; i++){
        if(fd_dev[i] > 0){
            close(fd_dev[i]);
        }
    }

    return ret;
}

int send_to_uart(int fd, unsigned char value)
{
    int ret = 0;
    unsigned char read_buffer[4] = {0};
    uart_debug("%s enter.\n", __func__);
    ret = write(fd, &value, 1);
    if(ret < 0){
        uart_debug("Error: write file %d failed.\n", fd);
        return ret;
    }

    int i = 0;
    for(i = 0; i < 20; i++){
        memset(read_buffer, 0, sizeof(read_buffer));
        ret = read(fd, &read_buffer, 4);
        if(ret < 0){
            // uart_debug("Error: read file %d failed.\n", fd);
            //return ret;
        }else{
            if(value >= 1 && value <= 127){//timeout
                if(ret == 1){
                    if(read_buffer[0] == value){
                        uart_debug("set timeout successful, timeout = %d.\n", value*10);
                        break;          
                    }else{
                        uart_debug("read_buffer, %d, %d, %d, %d.\n", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);           
                    }
                }

            }else if(value >= 128 && value <= 254){//feed dog
                if(ret == 3){
                    if(read_buffer[0] == value + 1){
                        uart_debug("set feed_dog successful, feed_value = %d.\n", value);
                        break;          
                    }else{
                        uart_debug("read_buffer, %d, %d, %d, %d.\n", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);           
                    }
                }
            }else if(value == 255){//reset watch dog
                if(ret == 1){
                    if(read_buffer[0] == 0){
                        uart_debug("reset watch dog successful\n");
                        break;          
                    }else{
                        uart_debug("read_buffer, %d, %d, %d, %d.\n", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);           
                    }
                }
            }
        }
        usleep(1000);
    }

    uart_debug("i = %d.\n", i);
    return 0;
}

int auto_scan(int *fd, unsigned char value)
{
    FILE *fp;
    char tmp_buffer[500] = {0};
    uart_debug("%s enter.\n", __func__);
    fp = popen("ls /dev/ttyUSB*", "r");
    fread(tmp_buffer, sizeof(tmp_buffer), 1, fp);
    uart_debug("%s.\n", tmp_buffer);
    uart_debug("strlen(tmp_buffer) = %d.\n", (int)strlen(tmp_buffer));
    pclose(fp);

    char * index = tmp_buffer;
    int count = 0;//the num of uart device
    int fd_tmp;
    int j = 0;
    char tmp_str[50] = {0};
    // uart_debug("&index = %d.\n", (long)(index));
    index = strstr(index, "/dev/ttyUSB");
    while(index != NULL){
        // uart_debug("&index = %d.\n", (long)(index));
        memset(tmp_str, 0, sizeof(tmp_str));
        for(j = 0; j < 50; j++){
            if(*(index+j) == 10){//uart_debug("bmp_buffer[i]");
                break;
            }
            tmp_str[j] = *(index+j);
        }
        tmp_str[j] = '\0';
        // uart_debug("j = %d\n", j);
        uart_debug("%s\n", tmp_str);

        fd_tmp = init_uart(tmp_str, DEFAULT_BAUDRATE);
        if(fd_tmp < 0)
        {
            // uart_debug("Error: set buad failed.\n");
            // close(fd_tmp);
            //return -1;
        } else {
            // uart_debug("set buad successful.\n");
            // if(feed_dog(fd_tmp, FEED_VALUE) == 0){
            if(send_to_uart(fd_tmp, value) == 0){
			#if 1 //关闭24小时硬重启
				send_to_uart(fd_tmp, 0xf3) ==0{
				uart_debug("[ %s ] : ****DISABLE_REBOOT_24R: %s ****\n", get_time(), tmp_str);
				
                write_log("DISABLE_REBOOT_24R, %s ****\n", tmp_str);	
				}
			#endif
                uart_debug("[ %s ] : ****watch_dog is connected: %s ****\n", get_time(), tmp_str);
				
                write_log("watch_dog is connected, %s ****\n", tmp_str);
                if(count < 20){
                    fd[count] = fd_tmp;
                    count++;
                }else{
                    uart_debug("Error: too much equipment.\n");
                    write_log("Error: too much equipment.\n");
                }
            }
        }   

        index += 8;
        index = strstr(index, "/dev/ttyUSB");
        // usleep(1000);
    }    
    uart_debug("count = %d.\n", count);
    return count;
}

// int write_log(int fd, char *str);

char *get_time(void)
{
    time_t now;
    struct tm *timenow;
    char *str;
    time(&now);
    timenow = localtime(&now);
    // uart_debug("%s/n",asctime(timenow));
    str = asctime(timenow);
    *(str+strlen(str)-1) = 0;//remove \n
    return str;
}


// char *get_time(void)
// {
//     char buffer[100] = {0};
//     char *str = buffer;
//     time_t now;
//     time(&now);
//     struct tm *local;
//     local = localtime(&now);
 
//     sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d", local->tm_year+1900, local->tm_mon,
//                 local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
//     // printf("%s\n", str);
//     return str;
// }



int set_timeout(int fd, unsigned char timeout)
{
    int ret;
    unsigned char read_val;
    uart_debug("%s enter.\n", __func__);
    if(timeout < 1 | timeout > 127){
        uart_debug("Error: timeout is not 1-127.\n");
        return -1;
    }
    ret = write(fd, &timeout, 1);
    if(ret < 0){
        uart_debug("Error: write file %d failed.\n", fd);
        return ret;
    }

    int i = 0;
    for(i = 0; i < 20; i++){
        ret = read(fd, &read_val, 1);
        if(ret < 0){
            uart_debug("Error: read file %d failed.\n", fd);
            //return ret;
        }else{
            uart_debug("read file %d successful.\n", fd);
            break;
        }
        usleep(1000);
    }
    if(read_val == timeout){
        uart_debug("set timeout successful. timeout = %d.\n", timeout);
    }else{
        uart_debug("Error: read read_val and write timeout is not equal.\n");
        return -1;
    }
    return 0;
}

int feed_dog(int fd, unsigned char feed_val)
{
    int ret = 0;
    unsigned char read_buffer[4] = {0};
    uart_debug("%s enter.\n", __func__);
    ret = write(fd, &feed_val, 1);
    if(ret < 0){
        uart_debug("Error: write file %d failed.\n", fd);
        return ret;
    }

    int i = 0;
    for(i = 0; i < 20; i++){
		memset(read_buffer, 0, sizeof(read_buffer));
        ret = read(fd, &read_buffer, 4);
        if(ret < 0){
            // uart_debug("Error: read file %d failed.\n", fd);
            //return ret;
        }else{
			if(read_buffer[0] == feed_val + 1){
            	uart_debug("set feed_val successful, feed_val = %d.\n", feed_val);
				break;			
			}else{
				uart_debug("read_buffer, %d, %d, %d, %d.\n", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);			
			}
        }
        usleep(1000);
    }

#if 0
    #if 0
    if(read_buffer[1] == feed_val + 0){
    #else
    if(read_buffer[1] == feed_val + 1){
    #endif
        uart_debug("set feed_val successful, feed_val = %d.\n", feed_val);
    }else{
        uart_debug("Error: read read_val and write timeout is not equal.read_val = %d, feed_val = %d.\n", read_val, feed_val);
        return -1;
    }
#endif
    return 0;
}


void write_log(const char* ms, ... )
{
    char wzLog[1024] = {0};
    char buffer[1024] = {0};
    va_list args;
    va_start(args, ms);
    vsprintf( wzLog ,ms,args);
    va_end(args);
 
    char *str = get_time();
    sprintf(buffer, "[ %s ] : %s", str, wzLog);
    FILE* file = fopen(log_file_path,"a+");
    fwrite(buffer,1,strlen(buffer),file);
    fclose(file);
 
//  syslog(LOG_INFO,wzLog);
    return ;
}


    // time_t now;
    // time(&now);
    // struct tm *local;
    // local = localtime(&now);
    // printf("%04d-%02d-%02d %02d:%02d:%02d %s\n", local->tm_year+1900, local->tm_mon,
    //         local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
    //         wzLog);
    // sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d %s\n", local->tm_year+1900, local->tm_mon,
    //             local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
    //             wzLog);
