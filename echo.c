/*
 * Simple Echo pseudo-device KLD
 *
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/filedesc.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/priv.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/ttycom.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/serialize.h>

#include <sys/thread2.h>
#include <sys/mplock2.h>

#define BUFFERSIZE 255

/* Function prototypes */
static d_open_t		echo_open;
static d_close_t	echo_close;
static d_read_t		echo_read;
static d_write_t	echo_write;

/* Character device entry points */
static struct dev_ops echo_ops = {
	.head.name = "echo",
	.d_open = echo_open,
	.d_close = echo_close,
	.d_read = echo_read,
	.d_write = echo_write,
};

struct s_echo {
	char msg[BUFFERSIZE + 1];
	int len;
};

/* vars */
static struct cdev *echodev;
static struct s_echo *echomsg;

MALLOC_DECLARE(M_ECHOBUF);
MALLOC_DEFINE(M_ECHOBUF, "echobuffer", "buffer for echo module");

/*
 * This function is called by the kld[un]load(2) system calls to
 * determine what actions to take when a module is loaded or unloaded.
 */
static int
echo_loader(struct module *m __unused, int event, void *arg __unused)
{
	int error = 0;
	
	switch(event) {
	case MOD_LOAD: /* kldload */
		echodev = make_dev(&echo_ops,
			0, UID_ROOT, GID_WHEEL, 0600, "echo");
		if (!echodev) {
			return -1;
		}
		
		echomsg = kmalloc(sizeof(*echomsg), M_ECHOBUF, M_WAITOK | M_ZERO);
		kprintf("Echo device loaded.\n");
		break;
	case MOD_UNLOAD:
		destroy_dev(echodev);
		kfree(echomsg, M_ECHOBUF);
		kprintf("Echo device unloaded.\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}
	return error;
}

static int
echo_open(struct dev_open_args __unused *ap)
{
	int error = 0;

	uprintf("Opened device \"echo\" successfully.\n");
	return error;
}

static int
echo_close(struct dev_close_args __unused *ap)
{

	uprintf("Closing device \"echo\".\n");
	return 0;
}

static int
echo_read(struct dev_read_args *ap)
{
	size_t amount;
	int error = 0;

	/*
	 * How big is this read operation? Either as big as the user wants,
	 * or as big as the remaining data. Note that the 'len' does not
	 * include the trailing null character.
	 */
	amount = MIN(ap->a_uio->uio_resid,
		ap->a_uio->uio_offset >= echomsg->len + 1 ? 0 :
			echomsg->len + 1 - ap->a_uio->uio_offset);

	if((error = uiomove(echomsg->msg, amount, ap->a_uio)))
	{
		uprintf("uiomove failed!\n");
	}

	return error;
}

static int
echo_write(struct dev_write_args *ap)
{
	size_t amount;
	int error = 0;

	struct uio *the_uio = ap->a_uio;

	/*
	 * We either write from the beginning or are appending -- do
	 * not allow random access.
	 */
	if (the_uio->uio_offset != 0 && the_uio->uio_offset != echomsg->len)
	{
		return EINVAL;
	}

	/* This is a new message, reset length */
	if (the_uio->uio_offset == 0)
		echomsg->len = 0;

	/* Copy the string in from user memory to kernel memory. */
	amount = MIN(the_uio->uio_resid, (BUFFERSIZE - echomsg->len));

	error = uiomove(echomsg->msg + the_uio->uio_offset, amount, the_uio);

	/* Now we need to null terminate and record the length */
	echomsg->len = the_uio->uio_offset;
	echomsg->msg[echomsg->len] = 0;

	if (error != 0)
		uprintf("Write failed: bad address!\n");

	return error;
}

DEV_MODULE(echo, echo_loader, NULL);
