/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Major definido por el enunciado */
#define SYNCWRITE_MAJOR 65

typedef struct write_entry {
  struct write_entry *next;
  char *data;
  size_t len;
  ssize_t ret;         /* valor que writer retornará */
  KCondition cond;     /* condición para despertar al escritor */
  int minor;           /* 0 = non-prio, 1 = prio */
} write_entry;

/* Colas FIFO para prioridad y no prioridad */
static write_entry *prio_head = NULL, *prio_tail = NULL;
static write_entry *norm_head = NULL, *norm_tail = NULL;

/* Mutex que protege las colas */
static KMutex qmutex;

/* utilidades de manejo de colas */
static void enqueue(write_entry **head, write_entry **tail, write_entry *w) {
  w->next = NULL;
  if (*tail == NULL) {
    *head = *tail = w;
  } else {
    (*tail)->next = w;
    *tail = w;
  }
}

static write_entry *dequeue(write_entry **head, write_entry **tail) {
  write_entry *w = *head;
  if (w) {
    *head = w->next;
    if (*head == NULL) *tail = NULL;
    w->next = NULL;
  }
  return w;
}

static int remove_entry(write_entry **head, write_entry **tail, write_entry *w) {
  write_entry **p = head;
  while (*p) {
    if (*p == w) {
      *p = w->next;
      if (*p == NULL) *tail = NULL;
      return 0;
    }
    p = &((*p)->next);
  }
  return -1;
}

/* file operations */
static ssize_t syncwrite_read(struct file *filp, char __user *buf, size_t count, loff_t *offp) {
  ssize_t copied = 0;
  write_entry *w = NULL;

  m_lock(&qmutex);
  /* elegir prioridad primero */
  if (prio_head != NULL) {
    w = dequeue(&prio_head, &prio_tail);
  } else if (norm_head != NULL) {
    w = dequeue(&norm_head, &norm_tail);
  }

  if (w == NULL) {
    /* No hay escrituras pendientes -> EOF */
    m_unlock(&qmutex);
    return 0;
  }

  /* hay una escritura: copiar datos al usuario */
  copied = (count < w->len) ? count : w->len;
  if (copy_to_user(buf, w->data, copied)) {
    /* En caso de error, reencolar? mejor liberar y devolver -EFAULT */
    kfree(w->data);
    kfree(w);
    m_unlock(&qmutex);
    return -EFAULT;
  }

  /* indicar al escritor cuantos bytes se leyeron (podemos devolver len)
   * y despertar al escritor. El escritor retornará su tamaño original.
   */
  w->ret = w->len; /* el escritor devuelve el tamaño escrito */

  /* Señalamos al escritor que su escritura fue consumida */
  c_signal(&w->cond);

  /* Al liberar el mutex (m_unlock) se despertará al escritor (ver kmutex.c) */
  m_unlock(&qmutex);

  /* liberamos el buffer del kernel; el escritor ya podrá retornar */
  kfree(w->data);
  /* No liberar 'w' aquí: el escritor lo hará cuando despierte. */

  return copied;
}

static ssize_t syncwrite_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp) {
  int minor = iminor(file_inode(filp));
  write_entry *w;
  int rc;

  if (count == 0)
    return 0;

  w = kmalloc(sizeof(write_entry), GFP_KERNEL);
  if (!w) return -ENOMEM;
  memset(w, 0, sizeof(*w));

  w->data = kmalloc(count, GFP_KERNEL);
  if (!w->data) { kfree(w); return -ENOMEM; }
  if (copy_from_user(w->data, buf, count)) {
    kfree(w->data);
    kfree(w);
    return -EFAULT;
  }
  w->len = count;
  w->ret = 0;
  w->minor = minor;
  c_init(&w->cond);

  m_lock(&qmutex);
  /* encolar por prioridad */
  if (minor == 1) {
    enqueue(&prio_head, &prio_tail, w);
  } else {
    enqueue(&norm_head, &norm_tail, w);
  }

  /* Esperar hasta que un lector consuma esta escritura. c_wait es
   * interruptible: si el proceso recibe señal retorna -EINTR.
   * c_wait re-adquiere el mutex antes de retornar. */
  rc = c_wait(&w->cond, &qmutex);
  if (rc == -EINTR) {
    /* El escritor fue interrumpido: hay que eliminarlo de la cola si
     * aún está en ella, y liberar recursos. */
    remove_entry((w->minor==1?&prio_head:&norm_head), (w->minor==1?&prio_tail:&norm_tail), w);
    kfree(w->data);
    kfree(w);
    m_unlock(&qmutex);
    return -EINTR;
  }

  /* c_wait retornó 0 y el mutex está actualmente adquirido. El lector
   * dejó en w->ret el valor que debe retornar el escritor. */
  rc = (ssize_t) w->ret;

  /* Ya no necesitamos la estructura write_entry: liberarla y devolver rc */
  kfree(w);
  m_unlock(&qmutex);
  return rc;
}

static int syncwrite_open(struct inode *inode, struct file *filp) {
  try_module_get(THIS_MODULE);
  return 0;
}

static int syncwrite_release(struct inode *inode, struct file *filp) {
  module_put(THIS_MODULE);
  return 0;
}

static const struct file_operations syncwrite_fops = {
  .owner = THIS_MODULE,
  .read = syncwrite_read,
  .write = syncwrite_write,
  .open = syncwrite_open,
  .release = syncwrite_release,
};

static int __init syncwrite_init(void) {
  int res;

  printk("syncwrite: Inserting syncwrite module\n");

  m_init(&qmutex);

  res = register_chrdev(SYNCWRITE_MAJOR, "syncwrite", &syncwrite_fops);
  if (res < 0) {
    printk(KERN_ERR "syncwrite: cannot register device with major %d\n", SYNCWRITE_MAJOR);
    return res;
  }
  return 0;
}

static void __exit syncwrite_exit(void) {
  unregister_chrdev(SYNCWRITE_MAJOR, "syncwrite");
  printk("syncwrite: Removing syncwrite module\n");

  /* limpiar colas si hubiese entradas (defensivo) */
  while (prio_head) {
    write_entry *w = dequeue(&prio_head, &prio_tail);
    kfree(w->data);
    kfree(w);
  }
  while (norm_head) {
    write_entry *w = dequeue(&norm_head, &norm_tail);
    kfree(w->data);
    kfree(w);
  }
}

module_init(syncwrite_init);
module_exit(syncwrite_exit);
