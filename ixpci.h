/* Declarations for PCI DAQ series.*/
#ifndef _IXPCI_H
#define _IXPCI_H

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/version.h>

#define ICPDAS_LICENSE "GPL"

/* General Definition */
#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#define ORGANIZATION "icpdas"
#define FAMILY "ixpci"			/* name of family */
#define DEVICE_NAME "ixpci"		/* device name used in /dev and /proc */
#define DEVICE_NAME_LEN 5
#define DEVICE_NR_DEVS 4
#define DEVICE_MAJOR 0			/* dynamic allocation of major number */
#define DEVICE_MINOR 0			

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
#define IXPCI_PROC_FILE "/proc/ixpci/ixpci"
#else
#define IXPCI_PROC_FILE "/proc/ixpci"
#endif

#define PCI_BASE_ADDRESSES_NUMBER  6
#define PBAN  PCI_BASE_ADDRESSES_NUMBER

#define CARD_NAME_LENGTH  32
#define CNL  CARD_NAME_LENGTH

#define KMSG(fmt, args...) printk(KERN_INFO FAMILY ": " fmt, ## args)

/* PCI Card's ID (vendor id).(device id) */
/*
          0x 1234 5678 1234 5678
             ---- ---- ---- ----
              |    |     |    |
      vendor id    |     |    sub-device id
	       device id     sub-vendor id
*/

#define PCI_1800      0x1234567800000000
#define PCI_1802      0x1234567800000000
#define PCI_1602      0x1234567800000000
#define PCI_1602_A    0x1234567600000000
#define PCI_1202      0x1234567200000000
#define PCI_1002      0x12341002c1a20823
#define PCI_P16C16    0x12341616c1a20823
#define PCI_P16R16    0x12341616c1a20823
#define PCI_P16POR16  0x12341616c1a20823
#define PCI_P8R8      0x12340808c1a20823
#define PCI_TMC12     0x10b5905021299912
#define PCI_M512      0x10b5905021290512
#define PCI_M256      0x10b5905021290256
#define PCI_M128      0x10b5905021290128
#define PCI_9050EVM   0x10b5905010b59050
/*
#define PCI_1800      0x1234567800000000	
#define PCI_1802      0x1234567800000000	
#define PCI_1602      0x1234567800000000	
#define PCI_1602_A    0x1234567600000000	
#define PCI_1202      0x1234567200000000
#define PCI_1002      0x12341002c1a20823
#define PCI_P16C16    0x12341616c1a20823	
#define PCI_P16R16    0x12341616c1a20823	
#define PCI_P16POR16  0x12341616c1a20823	
#define PCI_P8R8      0x12340808c1a20823
#define PCI_TMC12     0x10b5905021299912
#define PCI_M512      0x10b5905021290512
#define PCI_M256      0x10b5905021290256
#define PCI_M128      0x10b5905021290128
#define PCI_9050EVM   0x10b5905010b59050
*/

#define IXPCI_VENDOR(a)		((a) >> 48)
#define IXPCI_DEVICE(a)		(((a) >> 32) & 0x0ffff)
#define IXPCI_SUBVENDOR(a)	(((a) >> 16) & 0x0ffff)
#define IXPCI_SUBDEVICE(a)	((a) & 0x0ffff)

/* The chaos of name convention from hardware manual... */
enum {
	IXPCI_8254_COUNTER_0,
	IXPCI_8254_COUNTER_1,
	IXPCI_8254_COUNTER_2,
	IXPCI_8254_CONTROL_REG,
	IXPCI_SELECT_THE_ACTIVE_8254_CHIP,
	IXPCI_GENERAL_CONTROL_REG,
	IXPCI_STATUS_REG,
	IXPCI_AD_SOFTWARE_TRIGGER_REG,
	IXPCI_DIGITAL_INPUT_PORT,
	IXPCI_DIGITAL_OUTPUT_PORT,
	IXPCI_ANALOG_INPUT_CHANNEL_CONTROL_REG,
	IXPCI_ANALOG_INPUT_GAIN_CONTROL_REG,
	IXPCI_ANALOG_INPUT_PORT,
	IXPCI_ANALOG_OUTPUT_CHANNEL_1,
	IXPCI_ANALOG_OUTPUT_CHANNEL_2,
	IXPCI_PCI_INTERRUPT_CONTROL_REG,
	IXPCI_CLEAR_INTERRUPT,
	IXPCI_LAST_REG
};

#define IXPCI_8254C0       IXPCI_8254_COUNTER_0
#define IXPCI_8254C1       IXPCI_8254_COUNTER_1
#define IXPCI_8254C2       IXPCI_8254_COUNTER_2
#define IXPCI_8254CR       IXPCI_8254_CONTROL_REG
#define IXPCI_8254_CHIP_SELECT IXPCI_SELECT_THE_ACTIVE_8254_CHIP
#define IXPCI_8254CS       IXPCI_8254_CHIP_SELECT
#define IXPCI_GCR          IXPCI_GENERAL_CONTROL_REG
#define IXPCI_CONTROL_REG  IXPCI_GENERAL_CONTROL_REG
#define IXPCI_CR           IXPCI_CONTROL_REG
#define IXPCI_SR           IXPCI_STATUS_REG
#define IXPCI_ADST         IXPCI_AD_SOFTWARE_TRIGGER_REG
#define IXPCI_DI           IXPCI_DIGITAL_INPUT_PORT
#define IXPCI_DO           IXPCI_DIGITAL_OUTPUT_PORT
#define IXPCI_AICR         IXPCI_ANALOG_INPUT_CHANNEL_CONTROL_REG
#define IXPCI_AIGR         IXPCI_ANALOG_INPUT_GAIN_CONTROL_REG
#define IXPCI_AI           IXPCI_ANALOG_INPUT_PORT
#define IXPCI_AD           IXPCI_AI
#define IXPCI_AO1          IXPCI_ANALOG_OUTPUT_CHANNEL_1
#define IXPCI_DA1          IXPCI_AO1
#define IXPCI_AO2          IXPCI_ANALOG_OUTPUT_CHANNEL_2
#define IXPCI_DA2          IXPCI_AO2
#define IXPCI_PICR         IXPCI_PCI_INTERRUPT_CONTROL_REG
#define IXPCI_CI           IXPCI_CLEAR_INTERRUPT

/* IXPCI structure for signal conditions */
typedef struct ixpci_signal {
	int sid;					/* signal id */
	pid_t pid;					/* process id */
	struct task_struct *task;	/* pointer to task structure */
	int is;						/* mask for irq source 0 disable 1 enable */
	int edge;					/* active edge for each irq source 0 for
								   negative (falling) edge 1 for positive
								   (rising) edge */
	int bedge;					/* both edges, or bipolar. 0 up to the
								   setting in variable edge. 1 does action 
								   for both negative and positive
								   edges, discards setting in variable
								   edge.  */
} ixpci_signal_t;

/* IXPCI structure for register */
typedef struct ixpci_reg {
	unsigned int id;			/* register's id */
	unsigned int value;			/* register's value for read/write */
	int mode;					/* operation mode */
} ixpci_reg_t;

/* register operation mode */
enum {
	IXPCI_RM_RAW,				/* read/write directly without data mask */
	IXPCI_RM_NORMAL,			/* read/write directly */
	IXPCI_RM_READY,				/* blocks before ready */
	IXPCI_RM_TRIGGER,			/* do software trigger before ready (blocked) */
	IXPCI_RM_LAST_MODE
};

/* IXPCI cards' definition */
struct ixpci_carddef {
	__u64 id;						/* composed sub-ids */
	unsigned int present;		/* card's present counter */
	char *module;				/* module name, if card is present then
								   load module in this name */
	char *name;					/* card's name */
};

extern struct ixpci_carddef ixpci_card[];

/* IXPCI device information for found cards' list */
typedef struct ixpci_devinfo {
	struct ixpci_devinfo *next;	/* next device (ixpci card) */
	struct ixpci_devinfo *prev;	/* previous device */
	struct ixpci_devinfo *next_f;	/* next device in same family */
	struct ixpci_devinfo *prev_f;	/* previous device in same family */
	unsigned int no;			/* device number (minor number) */
	__u64 id;		/* card's id */
	unsigned int irq;			/* interrupt */
	unsigned long base[PBAN];	/* base I/O addresses */
	unsigned int range[PBAN];	/* ranges for each I/O address */
	unsigned int open;			/* open counter */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        struct cdev *cdev;
#else
        struct file_operations *fops;   /* file operations for this device */
#endif
	char name[CNL];				/* card name information */
	struct ixpci_signal sig;	/* user signaling for interrupt */
} ixpci_devinfo_t;

/* IOCTL command IDs */
enum {
	IXPCI_IOCTL_ID_RESET,
	IXPCI_IOCTL_ID_GET_INFO,
	IXPCI_IOCTL_ID_SET_SIG,
	IXPCI_IOCTL_ID_READ_REG,
	IXPCI_IOCTL_ID_WRITE_REG,
	IXPCI_IOCTL_ID_TIME_SPAN,
	IXPCI_IOCTL_ID_DI,
	IXPCI_IOCTL_ID_DO,
	IXPCI_IOCTL_ID_IRQ_ENABLE,
	IXPCI_IOCTL_ID_IRQ_DISABLE,
	IXPCI_IOCTL_ID_LAST_ITEM
};
/* IXPCI IOCTL command */
#define IXPCI_MAGIC_NUM  0x26	/* why? ascii codes 'P' + 'D' + 'A' + 'Q' */
#define IXPCI_GET_INFO   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_GET_INFO, ixpci_devinfo_t *)

#define IXPCI_SET_SIG    _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_SET_SIG, ixpci_signal_t *)

#define IXPCI_READ_REG   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_READ_REG, ixpci_reg_t *)
#define IXPCI_WRITE_REG  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_WRITE_REG, ixpci_reg_t *)
#define IXPCI_TIME_SPAN  _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_TIME_SPAN, int)
#define IXPCI_WAIT       IXPCI_TIME_SPAN
#define IXPCI_DELAY      IXPCI_TIME_SPAN
#define IXPCI_BLOCK      IXPCI_TIME_SPAN
#define IXPCI_RESET      _IO(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_RESET)

#define IXPCI_IOCTL_DI   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DI, void *)
#define IXPCI_IOCTL_DO   _IOR(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_DO, void *)

#define IXPCI_IRQ_ENABLE  _IO(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_IRQ_ENABLE)
#define IXPCI_IRQ_DISABLE  _IO(IXPCI_MAGIC_NUM, IXPCI_IOCTL_ID_IRQ_DISABLE)

/* Exported Symbols */
#ifdef __KERNEL__

/* from ixpcitmc12.o */
int ixpcitmc12_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcitmc12_release(struct inode *, struct file *);
#else
void ixpcitmc12_release(struct inode *, struct file *);
#endif
int ixpcitmc12_open(struct inode *, struct file *);

/* from ixpcip16x16.o */
int ixpcip16x16_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcip16x16_release(struct inode *, struct file *);
#else
void ixpcip16x16_release(struct inode *, struct file *);
#endif
int ixpcip16x16_open(struct inode *, struct file *);

/* from ixpcip8r8.o */
int ixpcip8r8_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpcip8r8_release(struct inode *, struct file *);
#else
void ixpcip8r8_release(struct inode *, struct file *);
#endif
int ixpcip8r8_open(struct inode *, struct file *);

/* from ixpci1800.o */
int ixpci1800_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1800_release(struct inode *, struct file *);
#else
void ixpci1800_release(struct inode *, struct file *);
#endif
int ixpci1800_open(struct inode *, struct file *);

/* from ixpci1202.o */
int ixpci1202_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1202_release(struct inode *, struct file *);
#else
void ixpci1202_release(struct inode *, struct file *);
#endif
int ixpci1202_open(struct inode *, struct file *);

/* from ixpci1002.o */
int ixpci1002_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1002_release(struct inode *, struct file *);
#else
void ixpci1002_release(struct inode *, struct file *);
#endif
int ixpci1002_open(struct inode *, struct file *);

/* from ixpci1602.o */
int ixpci1602_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
int ixpci1602_release(struct inode *, struct file *);
#else
void ixpci1602_release(struct inode *, struct file *);
#endif
int ixpci1602_open(struct inode *, struct file *);

/* from ixpci.o */
void *(_cardname) (__u64, int);
void *(_pci_cardname) (__u64);
void (ixpci_copy_devinfo) (ixpci_devinfo_t *, ixpci_devinfo_t *);
extern ixpci_devinfo_t *ixpci_dev;
extern int ixpci_major;

/* from _proc.o */
int ixpci_proc_init(void);
void ixpci_proc_exit(void);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define devfs_register_chrdev(a,b,c) module_register_chrdev(a,b,c)
#define devfs_unregister_chrdev(a,b) module_unregister_chrdev(a,b)
#define ixpci_init(a) init_module(a)
#define ixpci_cleanup(a) cleanup_module(a)
#endif

#endif							/* __KERNEL__ */

#endif							/* _IXPCI_H */

