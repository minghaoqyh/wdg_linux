#include <fcntl.h>
#include <stdio.h>
#include <termios.h>            /*termios, tcgetattr(), tcsetattr()*/
#include <errno.h>

int open_dev(const char *Dev)
{
    int fd = open( Dev, O_RDWR| O_NOCTTY | O_NONBLOCK);         //| O_NOCTTY | O_NDELAY
    if (-1 == fd)
    {
        perror("Can't Open Serial Port");
        return -1;
    }
    else
        return fd;
}
/**
*@brief  Set TTY device Baudrate
*@param  fd     Type: int
*@param  speed  Type: int
*@return  void
*/
int speed_arr[] = {B921600,B460800,B115200,B38400, B19200, B9600, B4800, B2400, B1200, B300,
                    B921600,B460800,B115200,B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {921600, 460800, 115200,38400,  19200,  9600,  4800,  2400,  1200,  300,
                    921600, 460800, 115200, 38400, 19200,  9600, 4800, 2400, 1200,  300, };
void set_speed(int fd, int speed)
{
    int   i = 0;
    int   status = 0;
    struct termios option = {0};
    tcgetattr(fd, &option);
    for ( i= 0;  i < (int)(sizeof(speed_arr) / sizeof(int));  i++)
    {
        if(speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&option, (speed_t)speed_arr[i]);
            cfsetospeed(&option, (speed_t)speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &option);
            if(status != 0)
            {
                printf("tcsetattr  status = %d\n",status);
                return;
            }
            tcflush(fd,TCIOFLUSH);
            printf("speed = %d\n", speed);
            return;
        } else {
            //printf("Error: buad is not support.\n");
    	}
    }
}


int set_databits(struct termios *p_options, int databits)
{
    switch (databits)
    {
    case 7:
        p_options->c_cflag |= CS7;
        break;
    case 8:
        p_options->c_cflag |= CS8;
        break;
    default:
        fprintf(stderr,"Unsupported data size\n"); 
        return (-1);
    }
    return 0;
}

int set_stopbits(struct termios *p_options, int stopbits)
{
    switch (stopbits)
    {
        case 1:
            p_options->c_cflag &= ~CSTOPB;
            break;
        case 2:
            p_options->c_cflag |= CSTOPB;
           break;
        default:
             fprintf(stderr,"Unsupported stop bits\n");
             return (-1);
    }
    return 0;
}

int set_parity(struct termios *p_options, int parity)
{
    switch (parity)
    {
        case 'n':
        case 'N':
            p_options->c_cflag &= ~PARENB;   /* Clear parity enable */
            p_options->c_iflag &= ~INPCK;     /* Clear parity checking */
            break;

        case 'o':
        case 'O':
            p_options->c_cflag |= (PARODD | PARENB); /* Use odd parity | Enable parity bit */
            p_options->c_iflag |= INPCK;             /* Enable parity checking */
            break;

        case 'e':
        case 'E':
            p_options->c_cflag |= PARENB;     /* Enable parity bit */
            p_options->c_cflag &= ~PARODD;    /* Clear odd parity (use even) */
            p_options->c_iflag |= INPCK;       /* Enable parity checking */
            break;

        case 'S':
        case 's':  /*as no parity*/
            p_options->c_cflag &= ~PARENB; /* Clear parity enable */
            p_options->c_cflag &= ~CSTOPB; /* 1 Stopbit */
            p_options->c_iflag |= INPCK; /* Enable parity checking */
            break;

        default:
            fprintf(stderr,"Unsupported parity\n");
            return (-1);
    }
    return 0;
}

/**
*@brief   Set TTY device databit length, stop-bit and parity-bit
*@param  fd     type:  int
*@param  databits type:  int
*@param  stopbits type:  int
*@param  parity  type:  int
*/
int set_data_mode(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
    if  ( tcgetattr( fd,&options)  !=  0) {
        perror("SetupSerial 1");
        return(-1);
    }
    options.c_cflag &= ~CSIZE;
    set_databits(&options, databits);
    set_stopbits(&options, stopbits);
    set_parity(&options, parity);

    tcflush(fd,TCIFLUSH);

    // Choosing Raw Input
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	
    //wangyang1@basewin add for special char 0x13, 0x11, 0x0D
    // <pre name="code" class="cpp">struct termios options;  
    // if  ( tcgetattr( fd,&options)  !=  0)  
    // {  
    //     perror("SetupSerial 1");  
    //     return (-1);  
    // }  
    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);  
    options.c_oflag &= ~OPOST;  
    options.c_cflag |= CLOCAL | CREAD;  
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  
    tcsetattr(fd,TCSAFLUSH,&options);  
    //wangyang1@basewin add

    //options.c_cc[VTIME] = 150; /* Set Timeout as 15 seconds*/
    //options.c_cc[VMIN] = 0; /* Update the options and do it NOW */

    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        perror("SetupSerial 3");
        return (-1);
    }
    printf("databits = %d\n", databits);
    printf("stopbits = %d\n", stopbits);
    printf("parity = %c\n", parity);
    return (0);
}

/*
 * Use default params to open TTY device
*/
int set_com_orginal_mod(int fd)
{
    struct termios options;
    if  ( tcgetattr( fd,&options)  !=  0) {
        perror("SetupSerial 1");
        return(-1);
    }
    tcflush(fd,TCIFLUSH);

    options.c_iflag = 0;
    options.c_cflag = 0;
    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    options.c_oflag  &= ~OPOST;   /*Output*/
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        perror("SetupSerial 3");
        return (-1);
    }
    return (0);

}

int set_com_option(int fd, int mod, int baudrate)
{
    if(1 == mod)
    {
        set_speed(fd,baudrate);
        if (set_data_mode(fd,8,1,'N') == -1)
        {
            perror("Set Parity Error\n");
            return -1;
        }
    }
    else
    {
        if(0 != set_com_orginal_mod(fd))
        {
            return -1;
        }
    }
    return 0;
}


int init_uart(const char *Dev, int baudrate)
{
    int fd_uart;
    fd_uart = open_dev(Dev);
    if(fd_uart > 0){
        // uart_debug("open %s successful.\n", Dev);
        if(0 != set_com_option(fd_uart, 1, baudrate))
        {
            // uart_debug("Error: set buad failed.\n");
            close(fd_uart);
            return -1;
        } else {

        }
    }
    return fd_uart;    
}