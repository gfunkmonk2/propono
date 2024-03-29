/*
 * propono-spinner.c
 * This file is part of propono
 *
 * Copyright (C) 2005 - Paolo Maggi 
 * Copyright (C) 2002-2004 Marco Pesenti Gritti
 * Copyright (C) 2004 Christian Persch
 * Copyright (C) 2000 - Eazel, Inc. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * This widget was originally written by Andy Hertzfeld <andy@eazel.com> for
 * Caja. It was then modified by Marco Pesenti Gritti and Christian Persch
 * for Epiphany.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "propono-spinner.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

/* Spinner cache implementation */

#define PROPONO_TYPE_SPINNER_CACHE		(propono_spinner_cache_get_type())
#define PROPONO_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), PROPONO_TYPE_SPINNER_CACHE, ProponoSpinnerCache))
#define PROPONO_SPINNER_CACHE_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST((klass), PROPONO_TYPE_SPINNER_CACHE, ProponoSpinnerCacheClass))
#define PROPONO_IS_SPINNER_CACHE(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), PROPONO_TYPE_SPINNER_CACHE))
#define PROPONO_IS_SPINNER_CACHE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), PROPONO_TYPE_SPINNER_CACHE))
#define PROPONO_SPINNER_CACHE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), PROPONO_TYPE_SPINNER_CACHE, ProponoSpinnerCacheClass))

typedef struct _ProponoSpinnerCache		ProponoSpinnerCache;
typedef struct _ProponoSpinnerCacheClass		ProponoSpinnerCacheClass;
typedef struct _ProponoSpinnerCachePrivate	ProponoSpinnerCachePrivate;

struct _ProponoSpinnerCacheClass
{
	GObjectClass parent_class;
};

struct _ProponoSpinnerCache
{
	GObject parent_object;

	/*< private >*/
	ProponoSpinnerCachePrivate *priv;
};

#define PROPONO_SPINNER_CACHE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_SPINNER_CACHE, ProponoSpinnerCachePrivate))

struct _ProponoSpinnerCachePrivate
{
	/* Hash table of GdkScreen -> ProponoSpinnerCacheData */
	GHashTable *hash;
};

typedef struct
{
	guint        ref_count;
	GtkIconSize  size;
	gint         width;
	gint         height;
	GdkPixbuf  **animation_pixbufs;
	guint        n_animation_pixbufs;
} ProponoSpinnerImages;

#define LAST_ICON_SIZE			GTK_ICON_SIZE_DIALOG + 1
#define SPINNER_ICON_NAME		"process-working"
#define SPINNER_FALLBACK_ICON_NAME	"mate-spinner"
#define PROPONO_SPINNER_IMAGES_INVALID	((ProponoSpinnerImages *) 0x1)

typedef struct
{
	GdkScreen          *screen;
	GtkIconTheme       *icon_theme;
	ProponoSpinnerImages *images[LAST_ICON_SIZE];
} ProponoSpinnerCacheData;

static void propono_spinner_cache_class_init	(ProponoSpinnerCacheClass *klass);
static void propono_spinner_cache_init		(ProponoSpinnerCache      *cache);

static GObjectClass *propono_spinner_cache_parent_class;

static GType
propono_spinner_cache_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		const GTypeInfo our_info =
		{
			sizeof (ProponoSpinnerCacheClass),
			NULL,
			NULL,
			(GClassInitFunc) propono_spinner_cache_class_init,
			NULL,
			NULL,
			sizeof (ProponoSpinnerCache),
			0,
			(GInstanceInitFunc) propono_spinner_cache_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "ProponoSpinnerCache",
					       &our_info, 0);
	}

	return type;
}

static ProponoSpinnerImages *
propono_spinner_images_ref (ProponoSpinnerImages *images)
{
	g_return_val_if_fail (images != NULL, NULL);

	images->ref_count++;

	return images;
}

static void
propono_spinner_images_unref (ProponoSpinnerImages *images)
{
	g_return_if_fail (images != NULL);

	images->ref_count--;
	if (images->ref_count == 0)
	{
		guint i;

		/* LOG ("Freeing spinner images %p for size %d", images, images->size); */

		for (i = 0; i < images->n_animation_pixbufs; ++i)
		{
			g_object_unref (images->animation_pixbufs[i]);
		}
		g_free (images->animation_pixbufs);

		g_free (images);
	}
}

static void
propono_spinner_cache_data_unload (ProponoSpinnerCacheData *data)
{
	GtkIconSize size;
	ProponoSpinnerImages *images;

	g_return_if_fail (data != NULL);

	/* LOG ("ProponoSpinnerDataCache unload for screen %p", data->screen); */

	for (size = GTK_ICON_SIZE_INVALID; size < LAST_ICON_SIZE; ++size)
	{
		images = data->images[size];
		data->images[size] = NULL;

		if (images != NULL && images != PROPONO_SPINNER_IMAGES_INVALID)
		{
			propono_spinner_images_unref (images);
		}
	}
}

static GdkPixbuf *
extract_frame (GdkPixbuf *grid_pixbuf,
	       int x,
	       int y,
	       int size)
{
	GdkPixbuf *pixbuf;

	if (x + size > gdk_pixbuf_get_width (grid_pixbuf) ||
	    y + size > gdk_pixbuf_get_height (grid_pixbuf))
	{
		return NULL;
	}

	pixbuf = gdk_pixbuf_new_subpixbuf (grid_pixbuf,
					   x, y,
					   size, size);
	g_return_val_if_fail (pixbuf != NULL, NULL);

	return pixbuf;
}

static GdkPixbuf *
scale_to_size (GdkPixbuf *pixbuf,
	       int dw,
	       int dh)
{
	GdkPixbuf *result;
	int pw, ph;

	g_return_val_if_fail (pixbuf != NULL, NULL);

	pw = gdk_pixbuf_get_width (pixbuf);
	ph = gdk_pixbuf_get_height (pixbuf);

	if (pw != dw || ph != dh)
	{
		result = gdk_pixbuf_scale_simple (pixbuf, dw, dh,
						  GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		return result;
	}

	return pixbuf;
}

static ProponoSpinnerImages *
propono_spinner_images_load (GdkScreen *screen,
			   GtkIconTheme *icon_theme,
			   GtkIconSize icon_size)
{
	ProponoSpinnerImages *images;
	GdkPixbuf *icon_pixbuf, *pixbuf;
	GtkIconInfo *icon_info = NULL;
	int grid_width, grid_height, x, y, requested_size, size, isw, ish, n;
	const char *icon;
	GSList *list = NULL, *l;

	/* LOG ("ProponoSpinnerCacheData loading for screen %p at size %d", screen, icon_size); */

	/* START_PROFILER ("loading spinner animation") */

	if (!gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
						icon_size, &isw, &ish))
		goto loser;
 
	requested_size = MAX (ish, isw);

	/* Load the animation. The 'rest icon' is the 0th frame */
	icon_info = gtk_icon_theme_lookup_icon (icon_theme,
						SPINNER_ICON_NAME,
						requested_size, 0);
	if (icon_info == NULL)
	{
		g_warning ("Throbber animation not found");

		/* If the icon naming spec compliant name wasn't found, try the old name */
		icon_info = gtk_icon_theme_lookup_icon (icon_theme,
							SPINNER_FALLBACK_ICON_NAME,
						        requested_size, 0);
		if (icon_info == NULL)
		{
			g_warning ("Throbber fallback animation not found either");
			goto loser;
	 	}
	}

	g_assert (icon_info != NULL);

	size = gtk_icon_info_get_base_size (icon_info);
	icon = gtk_icon_info_get_filename (icon_info);

	if (icon == NULL)
		goto loser;

	icon_pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
	gtk_icon_info_free (icon_info);
	icon_info = NULL;

	if (icon_pixbuf == NULL)
	{
		g_warning ("Could not load the spinner file");
		goto loser;
	}

	grid_width = gdk_pixbuf_get_width (icon_pixbuf);
	grid_height = gdk_pixbuf_get_height (icon_pixbuf);

	n = 0;
	for (y = 0; y < grid_height; y += size)
	{
		for (x = 0; x < grid_width ; x += size)
		{
			pixbuf = extract_frame (icon_pixbuf, x, y, size);

			if (pixbuf)
			{
				list = g_slist_prepend (list, pixbuf);
				++n;
			}
			else
			{
				g_warning ("Cannot extract frame (%d, %d) from the grid\n", x, y);
			}
		}
	}

	g_object_unref (icon_pixbuf);

	if (list == NULL)
		goto loser;

	/* g_assert (n > 0); */

	if (size > requested_size)
	{
		for (l = list; l != NULL; l = l->next)
		{
			l->data = scale_to_size (l->data, isw, ish);
		}
 	}

	/* Now we've successfully got all the data */
	images = g_new (ProponoSpinnerImages, 1);
	images->ref_count = 1;
 
	images->size = icon_size;
	images->width = images->height = requested_size;

	images->n_animation_pixbufs = n;
	images->animation_pixbufs = g_new (GdkPixbuf *, n);

	for (l = list; l != NULL; l = l->next)
	{
		g_assert (l->data != NULL);
		images->animation_pixbufs[--n] = l->data;
	}
	g_assert (n == 0);

	g_slist_free (list);

	/* STOP_PROFILER ("loading spinner animation") */
	return images;
 
loser:
	if (icon_info)
	{
		gtk_icon_info_free (icon_info);
 	}

	g_slist_foreach (list, (GFunc) g_object_unref, NULL);

	/* STOP_PROFILER ("loading spinner animation") */

	return NULL;
}

static ProponoSpinnerCacheData *
propono_spinner_cache_data_new (GdkScreen *screen)
{
	ProponoSpinnerCacheData *data;

	data = g_new0 (ProponoSpinnerCacheData, 1);

	data->screen = screen;
	data->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect_swapped (data->icon_theme,
				  "changed",
				  G_CALLBACK (propono_spinner_cache_data_unload),
				  data);

	return data;
}

static void
propono_spinner_cache_data_free (ProponoSpinnerCacheData *data)
{
	g_return_if_fail (data != NULL);
	g_return_if_fail (data->icon_theme != NULL);

	g_signal_handlers_disconnect_by_func (data->icon_theme,
					      G_CALLBACK (propono_spinner_cache_data_unload),
					      data);

	propono_spinner_cache_data_unload (data);

	g_free (data);
}

static ProponoSpinnerImages *
propono_spinner_cache_get_images (ProponoSpinnerCache *cache,
				GdkScreen         *screen,
				GtkIconSize        icon_size)
{
	ProponoSpinnerCachePrivate *priv = cache->priv;
	ProponoSpinnerCacheData *data;
	ProponoSpinnerImages *images;

	/* LOG ("Getting animation images for screen %p at size %d", screen, icon_size); */

	g_return_val_if_fail (icon_size >= 0 && icon_size < LAST_ICON_SIZE, NULL);

	/* Backward compat: "invalid" meant "native" size which doesn't exist anymore */
	if (icon_size == GTK_ICON_SIZE_INVALID)
	{
		icon_size = GTK_ICON_SIZE_DIALOG;
	}

	data = g_hash_table_lookup (priv->hash, screen);
	if (data == NULL)
	{
		data = propono_spinner_cache_data_new (screen);
		/* FIXME: think about what happens when the screen's display is closed later on */
		g_hash_table_insert (priv->hash, screen, data);
	}

	images = data->images[icon_size];
	if (images == PROPONO_SPINNER_IMAGES_INVALID)
	{
		/* Load failed, but don't try endlessly again! */
		return NULL;
	}

	if (images != NULL)
	{
		/* Return cached data */
		return propono_spinner_images_ref (images);
	}

	images = propono_spinner_images_load (screen, data->icon_theme, icon_size);

	if (images == NULL)
 	{
		/* Mark as failed-to-load */
		data->images[icon_size] = PROPONO_SPINNER_IMAGES_INVALID;
 
		return NULL;
 	}

	data->images[icon_size] = images;

	return propono_spinner_images_ref (images);
}

static void
propono_spinner_cache_init (ProponoSpinnerCache *cache)
{
	ProponoSpinnerCachePrivate *priv;

	priv = cache->priv = PROPONO_SPINNER_CACHE_GET_PRIVATE (cache);

	/* LOG ("ProponoSpinnerCache initialising"); */

	priv->hash = g_hash_table_new_full (g_direct_hash,
					    g_direct_equal,
					    NULL,
					    (GDestroyNotify) propono_spinner_cache_data_free);
}

static void
propono_spinner_cache_finalize (GObject *object)
{
	ProponoSpinnerCache *cache = PROPONO_SPINNER_CACHE (object); 
	ProponoSpinnerCachePrivate *priv = cache->priv;

	g_hash_table_destroy (priv->hash);

	/* LOG ("ProponoSpinnerCache finalised"); */

	G_OBJECT_CLASS (propono_spinner_cache_parent_class)->finalize (object);
}

static void
propono_spinner_cache_class_init (ProponoSpinnerCacheClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	propono_spinner_cache_parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = propono_spinner_cache_finalize;

	g_type_class_add_private (object_class, sizeof (ProponoSpinnerCachePrivate));
}

static ProponoSpinnerCache *spinner_cache = NULL;

static ProponoSpinnerCache *
propono_spinner_cache_ref (void)
{
	if (spinner_cache == NULL)
	{
		ProponoSpinnerCache **cache_ptr;

		spinner_cache = g_object_new (PROPONO_TYPE_SPINNER_CACHE, NULL);
		cache_ptr = &spinner_cache;
		g_object_add_weak_pointer (G_OBJECT (spinner_cache),
					   (gpointer *) cache_ptr);

		return spinner_cache;
	}

	return g_object_ref (spinner_cache);
}

/* Spinner implementation */

#define SPINNER_TIMEOUT 125 /* ms */

#define PROPONO_SPINNER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), PROPONO_TYPE_SPINNER, ProponoSpinnerPrivate))

struct _ProponoSpinnerPrivate
{
	GtkIconTheme       *icon_theme;
	ProponoSpinnerCache  *cache;
	GtkIconSize         size;
	ProponoSpinnerImages *images;
	guint               current_image;
	guint               timeout;
	guint               timer_task;
	guint               spinning : 1;
	guint               need_load : 1;
};

static void propono_spinner_class_init	(ProponoSpinnerClass *class);
static void propono_spinner_init		(ProponoSpinner      *spinner);

static GObjectClass *parent_class;

GType
propono_spinner_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		const GTypeInfo our_info =
		{
			sizeof (ProponoSpinnerClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) propono_spinner_class_init,
			NULL,
			NULL, /* class_data */
			sizeof (ProponoSpinner),
			0, /* n_preallocs */
			(GInstanceInitFunc) propono_spinner_init
		};

		type = g_type_register_static (GTK_TYPE_WIDGET,
					       "ProponoSpinner",
					       &our_info, 0);
	}

	return type;
}

static gboolean
propono_spinner_load_images (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	if (priv->need_load)
	{
		/* START_PROFILER ("propono_spinner_load_images") */

		priv->images =
			propono_spinner_cache_get_images (priv->cache,
							gtk_widget_get_screen (GTK_WIDGET (spinner)),
							priv->size);

		/* STOP_PROFILER ("propono_spinner_load_images") */

		priv->current_image = 0; /* 'rest' icon */
		priv->need_load = FALSE;
	}

	return priv->images != NULL;
}

static void
propono_spinner_unload_images (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	if (priv->images != NULL)
	{
		propono_spinner_images_unref (priv->images);
		priv->images = NULL;
	}

	priv->current_image = 0;
	priv->need_load = TRUE;
}

static void
icon_theme_changed_cb (GtkIconTheme *icon_theme,
		       ProponoSpinner *spinner)
{
	propono_spinner_unload_images (spinner);
	gtk_widget_queue_resize (GTK_WIDGET (spinner));
}

static void
propono_spinner_init (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv;

	priv = spinner->priv = PROPONO_SPINNER_GET_PRIVATE (spinner);

	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (spinner), GTK_NO_WINDOW);

	priv->cache = propono_spinner_cache_ref ();
	priv->size = GTK_ICON_SIZE_DIALOG;
	priv->spinning = FALSE;
	priv->timeout = SPINNER_TIMEOUT;
	priv->need_load = TRUE;
}

static int
propono_spinner_expose (GtkWidget      *widget,
		      GdkEventExpose *event)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (widget);
	ProponoSpinnerPrivate *priv = spinner->priv;
	ProponoSpinnerImages *images;
	GdkPixbuf *pixbuf;
	GdkGC *gc;
	int x_offset, y_offset, width, height;
	GdkRectangle pix_area, dest;

	if (!GTK_WIDGET_DRAWABLE (spinner))
	{
		return FALSE;
	}

	if (priv->need_load &&
	    !propono_spinner_load_images (spinner))
	{
		return FALSE;
	}

	images = priv->images;
	if (images == NULL)
	{
		return FALSE;
	}

	/* Otherwise |images| will be NULL anyway */
	g_assert (images->n_animation_pixbufs > 0);

	g_assert (priv->current_image >= 0 &&
		  priv->current_image < images->n_animation_pixbufs);

	pixbuf = images->animation_pixbufs[priv->current_image];

	g_assert (pixbuf != NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	/* Compute the offsets for the image centered on our allocation */
	x_offset = (widget->allocation.width - width) / 2;
	y_offset = (widget->allocation.height - height) / 2;

	pix_area.x = x_offset + widget->allocation.x;
	pix_area.y = y_offset + widget->allocation.y;
	pix_area.width = width;
	pix_area.height = height;

	if (!gdk_rectangle_intersect (&event->area, &pix_area, &dest))
	{
		return FALSE;
	}

	gc = gdk_gc_new (widget->window);
	gdk_draw_pixbuf (widget->window, gc, pixbuf,
			 dest.x - x_offset - widget->allocation.x,
			 dest.y - y_offset - widget->allocation.y,
			 dest.x, dest.y,
			 dest.width, dest.height,
			 GDK_RGB_DITHER_MAX, 0, 0);
	g_object_unref (gc);

	return FALSE;
}

static gboolean
bump_spinner_frame_cb (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	/* This can happen when we've unloaded the images on a theme
	 * change, but haven't been in the queued size request yet.
	 * Just skip this update.
	 */
	if (priv->images == NULL)
		return TRUE;

	priv->current_image++;
	if (priv->current_image >= priv->images->n_animation_pixbufs)
	{
		/* the 0th frame is the 'rest' icon */
		priv->current_image = MIN (1, priv->images->n_animation_pixbufs);
	}

	gtk_widget_queue_draw (GTK_WIDGET (spinner));

	/* run again */
	return TRUE;
}

/**
 * propono_spinner_start:
 * @spinner: a #ProponoSpinner
 *
 * Start the spinner animation.
 **/
void
propono_spinner_start (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	priv->spinning = TRUE;

	if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)) &&
			       priv->timer_task == 0 &&
			       propono_spinner_load_images (spinner))
	{
		/* the 0th frame is the 'rest' icon */
		priv->current_image = MIN (1, priv->images->n_animation_pixbufs);

		priv->timer_task = g_timeout_add_full (G_PRIORITY_LOW,
						       priv->timeout,
						       (GSourceFunc) bump_spinner_frame_cb,
						       spinner,
						       NULL);
	}
}

static void
propono_spinner_remove_update_callback (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	if (priv->timer_task != 0)
	{
		g_source_remove (priv->timer_task);
		priv->timer_task = 0;
	}
}

/**
 * propono_spinner_stop:
 * @spinner: a #ProponoSpinner
 *
 * Stop the spinner animation.
 **/
void
propono_spinner_stop (ProponoSpinner *spinner)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	priv->spinning = FALSE;
	priv->current_image = 0;

	if (priv->timer_task != 0)
	{
		propono_spinner_remove_update_callback (spinner);

		if (GTK_WIDGET_MAPPED (GTK_WIDGET (spinner)))
			gtk_widget_queue_draw (GTK_WIDGET (spinner));
	}
}

/*
 * propono_spinner_set_size:
 * @spinner: a #ProponoSpinner
 * @size: the size of type %GtkIconSize
 *
 * Set the size of the spinner.
 **/
void
propono_spinner_set_size (ProponoSpinner *spinner,
			GtkIconSize size)
{
	if (size == GTK_ICON_SIZE_INVALID)
	{
		size = GTK_ICON_SIZE_DIALOG;
	}

	if (size != spinner->priv->size)
	{
		propono_spinner_unload_images (spinner);

		spinner->priv->size = size;

		gtk_widget_queue_resize (GTK_WIDGET (spinner));
	}
}

#if 0
/*
* propono_spinner_set_timeout:
* @spinner: a #ProponoSpinner
* @timeout: time delay between updates to the spinner.
*
* Sets the timeout delay for spinner updates.
**/
void
propono_spinner_set_timeout (ProponoSpinner *spinner,
			   guint         timeout)
{
	ProponoSpinnerPrivate *priv = spinner->priv;

	if (timeout != priv->timeout)
	{
		propono_spinner_stop (spinner);

		priv->timeout = timeout;

		if (priv->spinning)
		{
			propono_spinner_start (spinner);
		}
	}
}
#endif

static void
propono_spinner_size_request (GtkWidget *widget,
			   GtkRequisition *requisition)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (widget);
	ProponoSpinnerPrivate *priv = spinner->priv;

	if ((priv->need_load &&
	     !propono_spinner_load_images (spinner)) ||
            priv->images == NULL)
	{
		requisition->width = requisition->height = 0;
		gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
						   priv->size,
						   &requisition->width,
					           &requisition->height);
		return;
	}

	requisition->width = priv->images->width;
	requisition->height = priv->images->height;

	/* FIXME fix this hack */
	/* allocate some extra margin so we don't butt up against toolbar edges */
	if (priv->size != GTK_ICON_SIZE_MENU)
	{
		requisition->width += 2;
		requisition->height += 2;
	}
}

static void
propono_spinner_map (GtkWidget *widget)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (widget);
	ProponoSpinnerPrivate *priv = spinner->priv;

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	if (priv->spinning)
	{
		propono_spinner_start (spinner);
	}
}

static void
propono_spinner_unmap (GtkWidget *widget)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (widget);

	propono_spinner_remove_update_callback (spinner);

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
propono_spinner_dispose (GObject *object)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (object);

	g_signal_handlers_disconnect_by_func
			(spinner->priv->icon_theme,
			 G_CALLBACK (icon_theme_changed_cb), spinner);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
propono_spinner_finalize (GObject *object)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (object);

	propono_spinner_remove_update_callback (spinner);
	propono_spinner_unload_images (spinner);

	g_object_unref (spinner->priv->cache);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
propono_spinner_screen_changed (GtkWidget *widget,
			      GdkScreen *old_screen)
{
	ProponoSpinner *spinner = PROPONO_SPINNER (widget);
	ProponoSpinnerPrivate *priv = spinner->priv;
	GdkScreen *screen;

	if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
	{
		GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, old_screen);
	}

	screen = gtk_widget_get_screen (widget);

	/* FIXME: this seems to be happening when then spinner is destroyed!? */
	if (old_screen == screen)
		return;

	/* We'll get mapped again on the new screen, but not unmapped from
	 * the old screen, so remove timeout here.
	 */
	propono_spinner_remove_update_callback (spinner);

	propono_spinner_unload_images (spinner);

	if (old_screen != NULL)
	{
		g_signal_handlers_disconnect_by_func
			(gtk_icon_theme_get_for_screen (old_screen),
			 G_CALLBACK (icon_theme_changed_cb), spinner);
	}

	priv->icon_theme = gtk_icon_theme_get_for_screen (screen);
	g_signal_connect (priv->icon_theme, "changed",
			  G_CALLBACK (icon_theme_changed_cb), spinner);
}

static void
propono_spinner_class_init (ProponoSpinnerClass *class)
{
	GObjectClass *object_class =  G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->dispose = propono_spinner_dispose;
	object_class->finalize = propono_spinner_finalize;

	widget_class->expose_event = propono_spinner_expose;
	widget_class->size_request = propono_spinner_size_request;
	widget_class->map = propono_spinner_map;
	widget_class->unmap = propono_spinner_unmap;
	widget_class->screen_changed = propono_spinner_screen_changed;

	g_type_class_add_private (object_class, sizeof (ProponoSpinnerPrivate));
}

/*
 * propono_spinner_new:
 *
 * Create a new #ProponoSpinner. The spinner is a widget
 * that gives the user feedback about network status with
 * an animated image.
 *
 * Return Value: the spinner #GtkWidget
 **/
GtkWidget *
propono_spinner_new (void)
{
	return GTK_WIDGET (g_object_new (PROPONO_TYPE_SPINNER, NULL));
}
