/*
 * propono-ssh.c
 * SSH Utilities for Propono
 * This file is part of propono
 *
 * Copyright (C) 2009 - Jonh Wendell <wendell@bani.com.br>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <glib/gi18n.h>
#include <mate-keyring.h>

#include "propono-ssh.h"
#include "propono-utils.h"
#include "pty_open.h"

#define SSH_READ_TIMEOUT 40   /* seconds */

#ifdef HAVE_GRANTPT
/* We only use this on systems with unix98 ptys */
#define USE_PTY 1
#endif

typedef enum {
  SSH_VENDOR_INVALID = 0,
  SSH_VENDOR_OPENSSH,
  SSH_VENDOR_SSH
} SSHClientVendor;

static SSHClientVendor vendor = SSH_VENDOR_INVALID;

static SSHClientVendor
get_ssh_client_vendor (void)
{
  char *ssh_stderr;
  char *args[3];
  gint ssh_exitcode;
  SSHClientVendor res = SSH_VENDOR_INVALID;
  
  args[0] = g_strdup (SSH_PROGRAM);
  args[1] = g_strdup ("-V");
  args[2] = NULL;
  if (g_spawn_sync (NULL, args, NULL,
		    G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL,
		    NULL, NULL,
		    NULL, &ssh_stderr,
		    &ssh_exitcode, NULL))
    {
      if (ssh_stderr == NULL)
	res = SSH_VENDOR_INVALID;
      else if ((strstr (ssh_stderr, "OpenSSH") != NULL) ||
	       (strstr (ssh_stderr, "Sun_SSH") != NULL))
	res = SSH_VENDOR_OPENSSH;
      else if (strstr (ssh_stderr, "SSH Secure Shell") != NULL)
	res = SSH_VENDOR_SSH;
      else
	res = SSH_VENDOR_INVALID;
    }
  
  g_free (ssh_stderr);
  g_free (args[0]);
  g_free (args[1]);
  
  return res;
}

static gboolean
wait_for_reply (int stdout_fd, GError **error)
{
  fd_set ifds;
  struct timeval tv;
  int ret;
  
  FD_ZERO (&ifds);
  FD_SET (stdout_fd, &ifds);
  
  tv.tv_sec = SSH_READ_TIMEOUT;
  tv.tv_usec = 0;
      
  ret = select (stdout_fd+1, &ifds, NULL, NULL, &tv);

  if (ret <= 0)
    {
      g_set_error_literal (error,
			   PROPONO_SSH_ERROR,
			   PROPONO_SSH_ERROR_TIMEOUT,
			   _("Timed out when logging into SSH host"));
      return FALSE;
    }
  return TRUE;
}

static char **
setup_ssh_commandline (const gchar *host,
		       gint port,
		       const gchar *username,
		       gchar **extra_arguments,
		       gchar **command)
{
  guint last_arg;
  gchar **args;
  int extra_size, command_size, i;

  extra_size = extra_arguments != NULL ? g_strv_length (extra_arguments) : 0;
  command_size = command != NULL ? g_strv_length (command) : 0;
  args = g_new0 (gchar *, 15 + extra_size + command_size);

  last_arg = 0;
  args[last_arg++] = g_strdup (SSH_PROGRAM);

  for (i=0; i<extra_size; i++)
    args[last_arg++] = g_strdup (extra_arguments[i]);

  if (vendor == SSH_VENDOR_OPENSSH)
    {
      args[last_arg++] = g_strdup ("-oForwardX11=no");
      args[last_arg++] = g_strdup ("-oForwardAgent=no");
      args[last_arg++] = g_strdup ("-oProtocol=2");
#ifndef USE_PTY
      args[last_arg++] = g_strdup ("-oBatchMode=yes");
#endif
    }
  else if (vendor == SSH_VENDOR_SSH)
    args[last_arg++] = g_strdup ("-x");

  args[last_arg++] = g_strdup ("-p");
  args[last_arg++] = g_strdup_printf ("%d", port);

  args[last_arg++] = g_strdup ("-l");
  args[last_arg++] = g_strdup (username);

  args[last_arg++] = g_strdup (host);

  for (i=0; i<command_size; i++)
    args[last_arg++] = g_strdup (command[i]);

  args[last_arg++] = NULL;

  return args;
}

static gboolean
spawn_ssh (char *args[],
           pid_t *pid,
           int *tty_fd,
           int *stdin_fd,
           int *stdout_fd,
           int *stderr_fd,
           GError **error)
{
#ifdef USE_PTY
  *tty_fd = pty_open(pid, PTY_REAP_CHILD, NULL,
		     args[0], args, NULL,
		     300, 300, 
		     stdin_fd, stdout_fd, stderr_fd);
  if (*tty_fd == -1)
    {
      g_set_error_literal (error,
			   PROPONO_SSH_ERROR,
			   PROPONO_SSH_ERROR_FAILED,
			   _("Unable to spawn ssh program"));
      return FALSE;
    }
#else
  GError *my_error;
  GPid gpid;
  
  *tty_fd = -1;

  my_error = NULL;
  if (!g_spawn_async_with_pipes (NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				 &gpid,
				 stdin_fd, stdout_fd, stderr_fd, &my_error))
    {
      g_set_error (error,
                   PROPONO_SSH_ERROR,
                   PROPONO_SSH_ERROR_FAILED,
                   _("Unable to spawn ssh program: %s"), my_error->message);
      g_error_free (my_error);
      return FALSE;
    }
  *pid = gpid;
#endif
  
  return TRUE;
}

static const gchar *
get_authtype_from_password_line (const char *password_line)
{
  return g_str_has_prefix (password_line, "Enter passphrase for key") ?
	  "publickey" : "password";
}

static char *
get_object_from_password_line (const char *password_line)
{
  char *chr, *ptr, *object = NULL;

  if (g_str_has_prefix (password_line, "Enter passphrase for key"))
    {
      ptr = strchr (password_line, '\'');
      if (ptr != NULL)
        {
	  ptr += 1;
	  chr = strchr (ptr, '\'');
	  if (chr != NULL)
	    {
	      object = g_strndup (ptr, chr - ptr);
	    }
	  else
	    {
	      object = g_strdup (ptr);
	    }
	}
    }
  return object;
}

static gboolean
get_hostname_and_fingerprint_from_line (const gchar *buffer,
                                        gchar      **hostname_out,
                                        gchar      **fingerprint_out)
{
  gchar *pos;
  gchar *startpos;
  gchar *endpos;
  gchar *hostname = NULL;
  gchar *fingerprint = NULL;
  
  if (g_str_has_prefix (buffer, "The authenticity of host '"))
    {
      /* OpenSSH */
      pos = strchr (&buffer[26], '\'');
      if (pos == NULL)
        return FALSE;

      hostname = g_strndup (&buffer[26], pos - (&buffer[26]));

      startpos = strstr (pos, " key fingerprint is ");
      if (startpos == NULL)
        {
          g_free (hostname);
          return FALSE;
        }

      startpos = startpos + 20;
      endpos = strchr (startpos, '.');
      if (endpos == NULL)
        {
          g_free (hostname);
          return FALSE;
        }
      
      fingerprint = g_strndup (startpos, endpos - startpos);
    }
  else if (strstr (buffer, "Key fingerprint:") != NULL)
    {
      /* SSH.com*/
      startpos = strstr (buffer, "Key fingerprint:");
      if (startpos == NULL)
        {
          g_free (hostname);
          return FALSE;
        }
      
      startpos = startpos + 18;
      endpos = strchr (startpos, '\r');
      fingerprint = g_strndup (startpos, endpos - startpos);
    }

  *hostname_out = hostname;
  *fingerprint_out = fingerprint;

  return TRUE;
}

static gboolean
handle_login (GtkWindow *parent,
	      const   gchar *host,
	      int     port,
	      const   gchar *user,
	      int     tty_fd,
	      int     stdout_fd,
	      int     stderr_fd,
              GError  **error)
{
  GInputStream *prompt_stream, *stdout_stream;
  GOutputStream *reply_stream;
  fd_set ifds;
  struct timeval tv;
  int ret;
  int prompt_fd;
  char buffer[1024];
  gsize len;
  gboolean ret_val, aborted, save_in_keyring;
  gsize bytes_written;
  const gchar *authtype;
  gchar *object, *password;
  MateKeyringResult result;
  GList *matches;
  MateKeyringNetworkPasswordData *found_item;

  object = password = NULL;
  authtype = NULL;
  ret_val = TRUE;
  aborted = save_in_keyring = FALSE;

  if (vendor == SSH_VENDOR_SSH) 
    prompt_fd = stderr_fd;
  else
    prompt_fd = tty_fd;

  prompt_stream = g_unix_input_stream_new (prompt_fd, FALSE);
  stdout_stream = g_unix_input_stream_new (stdout_fd, FALSE);
  reply_stream = g_unix_output_stream_new (tty_fd, FALSE);

  while (1)
    {
      FD_ZERO (&ifds);
      FD_SET (stdout_fd, &ifds);
      FD_SET (prompt_fd, &ifds);
      
      tv.tv_sec = SSH_READ_TIMEOUT;
      tv.tv_usec = 0;
      
      ret = select (MAX (stdout_fd, prompt_fd)+1, &ifds, NULL, NULL, &tv);
      
      if (ret <= 0)
        {
          g_set_error_literal (error,
	                       G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
        	               _("Timed out when logging in"));
          ret_val = FALSE;
          break;
        }

      if (FD_ISSET (stdout_fd, &ifds))  /* Got reply to our check */
        {
          len = g_input_stream_read (stdout_stream,
                                     buffer, sizeof (buffer) - 1,
                                     NULL, error);
          if (len == -1)
            {
              ret_val = FALSE;
              break;
            }

          if (len == 0) /* Error, let's exit and look at stderr */
            {
              ret_val = TRUE;
              break;
            }

          buffer[len] = 0;
          if (strncmp (buffer, PROPONO_SSH_CHECK, PROPONO_SSH_CHECK_LENGTH) == 0)
            break;
          else
            {
	      g_set_error_literal (error,
				   PROPONO_SSH_ERROR,
				   PROPONO_SSH_ERROR_PERMISSION_DENIED,
				   _("Permission denied"));
              ret_val = FALSE;
              break;
            }
        }

      g_assert (FD_ISSET (prompt_fd, &ifds));

      len = g_input_stream_read (prompt_stream,
                                 buffer, sizeof (buffer) - 1,
                                 NULL, error);
      if (len == -1)
        {
          ret_val = FALSE;
          break;
        }
      buffer[len] = 0;

      if (strncmp (buffer, "\r\n", 2) == 0)
        continue;

      g_free (password);
      password = NULL;

      if (g_str_has_suffix (buffer, "password: ") ||
          g_str_has_suffix (buffer, "Password: ") ||
          g_str_has_suffix (buffer, "Password:")  ||
          g_str_has_prefix (buffer, "Password for ") ||
          g_str_has_prefix (buffer, "Enter Kerberos password") ||
          g_str_has_prefix (buffer, "Enter passphrase for key"))
        {
	  authtype = get_authtype_from_password_line (buffer);
	  object = get_object_from_password_line (buffer);

	  /* Search password in the keyring */
	  result = mate_keyring_find_network_password_sync (user,		/* username */
							     NULL,		/* domain */
							     host,	        /* server */
							     object,		/* object */
							     "ssh",		/* protocol */
							     authtype,		/* authtype */
							     port,		/* port */
							     &matches);

	  /* If the password was not found in keyring then ask for it */
	  if (result != MATE_KEYRING_RESULT_OK || matches == NULL || matches->data == NULL)
	    {
	      gchar *full_host;
	      gboolean res;

	      full_host = g_strjoin ("@", user, host, NULL);
	      res = propono_utils_ask_credential (parent,
						  "SSH",
						  full_host,
						  FALSE,
						  TRUE,
						  0,
						  NULL,
						  &password,
						  &save_in_keyring);
	      g_free (full_host);
              if (!res)
                {
                  g_set_error_literal (error,
				       PROPONO_SSH_ERROR,
                                       PROPONO_SSH_ERROR_PASSWORD_CANCELED,
        	                       _("Password dialog canceled"));
                  ret_val = FALSE;
                  break;
                }
            }
	  else
	    {
	      found_item = (MateKeyringNetworkPasswordData *) matches->data;
	      password = g_strdup (found_item->password);
	      mate_keyring_network_password_list_free (matches);
	    }

          if (!g_output_stream_write_all (reply_stream,
                                          password, strlen (password),
                                          &bytes_written,
                                          NULL, NULL) ||
              !g_output_stream_write_all (reply_stream,
                                          "\n", 1,
                                          &bytes_written,
                                          NULL, NULL))
            {
              g_set_error_literal (error,
	                           G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
        	                   _("Could not send password"));
              ret_val = FALSE;
              break;
            }
        }
      else if (g_str_has_prefix (buffer, "The authenticity of host '") ||
               strstr (buffer, "Key fingerprint:") != NULL)
        {
	  const gchar *choices[] = {_("Log In Anyway"), _("Cancel Login"), NULL};
	  const gchar *choice_string;
	  gchar *hostname = NULL;
	  gchar *fingerprint = NULL;
	  gint choice;
	  gchar *message;

	  get_hostname_and_fingerprint_from_line (buffer, &hostname, &fingerprint);

	  message = g_strdup_printf (_("The identity of the remote computer (%s) is unknown.\n"
				       "This happens when you log in to a computer the first time.\n\n"
				       "The identity sent by the remote computer is %s. "
				       "If you want to be absolutely sure it is safe to continue, "
				       "contact the system administrator."),
				     hostname ? hostname : host, fingerprint);

	  g_free (hostname);
	  g_free (fingerprint);

	  if (!propono_utils_ask_question (NULL,
					   message,
					   (char **)choices,
					   &choice))
	    {
	      g_set_error_literal (error,
				   PROPONO_SSH_ERROR,
				   PROPONO_SSH_ERROR_PASSWORD_CANCELED,
				   _("Login dialog canceled"));
	      g_free (message);
	      ret_val = FALSE;
	      break;
	    }
	  g_free (message);

	  choice_string = (choice == 0) ? "yes" : "no";
	  if (!g_output_stream_write_all (reply_stream,
					  choice_string,
					  strlen (choice_string),
					  &bytes_written,
					  NULL, NULL) ||
	      !g_output_stream_write_all (reply_stream,
					  "\n", 1,
					  &bytes_written,
					  NULL, NULL))
	    {
	      g_set_error_literal (error,
				   PROPONO_SSH_ERROR,
				   PROPONO_SSH_ERROR_IO_ERROR,
				   _("Can't send host identity confirmation"));
	      ret_val = FALSE;
	      break;
	    }
	}
      else
        {
	  g_set_error_literal (error,
			       PROPONO_SSH_ERROR,
			       PROPONO_SSH_ERROR_PERMISSION_DENIED,
			       _("Permission denied"));
	  ret_val = FALSE;
	  break;
        }
    }
  
  if (ret_val && save_in_keyring)
    {
      /* Login succeed, save password in keyring */
      MateKeyringResult result;
      guint32            keyring_item_id;

      result = mate_keyring_set_network_password_sync (
                NULL,         /* default keyring */
                user,         /* user            */
                NULL,         /* domain          */
                host,         /* server          */
                object,       /* object          */
                "ssh",        /* protocol        */
                authtype,     /* authtype        */
                port,         /* port            */
                password,     /* password        */
                &keyring_item_id);

      if (result != MATE_KEYRING_RESULT_OK)
        propono_utils_show_error (_("Error saving the credentials on the keyring."),
				  mate_keyring_result_to_message (result),
				  parent);

    }

  g_free (object);
  g_object_unref (prompt_stream);
  g_object_unref (stdout_stream);
  g_object_unref (reply_stream);

  return ret_val;
}

static gchar *
get_username (const gchar *host, const gchar *username)
{
  gchar *pos;

  if (username)
    return g_strdup (username);

  pos = strchr (host, '@');
  if (pos)
    return g_strndup (host, pos - host);

  return g_strdup (g_get_user_name ());
}

static gchar *
get_hostname (const gchar *host)
{
  gchar *pos;

  pos = strchr (host, '@');
  if (pos)
    return g_strdup (pos+1);

  return g_strdup (host);
}

static gboolean
look_for_stderr_errors (GDataInputStream *error_stream, GError **error)
{
  char *line;

  while (1)
    {
      line = g_data_input_stream_read_line (error_stream, NULL, NULL, NULL);

      if (line == NULL)
        {
          return TRUE;
        }
    
      if (strstr (line, "Permission denied") != NULL)
        {
          g_set_error_literal (error,
	                       PROPONO_SSH_ERROR, PROPONO_SSH_ERROR_PERMISSION_DENIED,
        	               _("Permission denied"));
          return FALSE;
        }
      else if (strstr (line, "Name or service not known") != NULL)
        {
          g_set_error_literal (error,
	                       PROPONO_SSH_ERROR, PROPONO_SSH_ERROR_HOST_NOT_FOUND,
        	               _("Hostname not known"));
          return FALSE;
        }
      else if (strstr (line, "No route to host") != NULL)
        {
          g_set_error_literal (error,
	                       PROPONO_SSH_ERROR, PROPONO_SSH_ERROR_HOST_NOT_FOUND,
        	               _("No route to host"));
          return FALSE;
        }
      else if (strstr (line, "Connection refused") != NULL)
        {
          g_set_error_literal (error,
	                       PROPONO_SSH_ERROR, PROPONO_SSH_ERROR_PERMISSION_DENIED,
        	               _("Connection refused by server"));
          return FALSE;
        }
      else if (strstr (line, "Host key verification failed") != NULL) 
        {
          g_set_error_literal (error,
	                       PROPONO_SSH_ERROR, PROPONO_SSH_ERROR_FAILED,
        	               _("Host key verification failed"));
          return FALSE;
        }
      
      g_free (line);
    }

  return TRUE;
}

gboolean
propono_ssh_connect (GtkWindow *parent,
		     const gchar *hostname,
		     gint port,
		     const gchar *username,
		     gchar **extra_arguments,
		     gchar **command,
		     gint *tty,
		     GError **error)
{
  int tty_fd, stdin_fd, stdout_fd, stderr_fd;
  pid_t pid;
  gchar *user, *host, **args;
  gboolean res;
  GInputStream *is;
  GDataInputStream *error_stream;

  if (!hostname)
    return FALSE;

  if (vendor == SSH_VENDOR_INVALID)
    vendor = get_ssh_client_vendor ();
  if (vendor == SSH_VENDOR_INVALID)
    {
      g_set_error_literal (error,
			   PROPONO_SSH_ERROR,
			   PROPONO_SSH_ERROR_INVALID_CLIENT,
			   _("Unable to find a valid SSH program"));
      return FALSE;
    }

  if (port <= 0)
    port = 22;

  user = get_username (hostname, username);
  host = get_hostname (hostname);

  args = setup_ssh_commandline (host, port, user, extra_arguments, command);
  if (!spawn_ssh (args,
		  &pid,
		  &tty_fd, &stdin_fd, &stdout_fd, &stderr_fd,
		  error))
    {
      g_strfreev (args);
      g_free (user);
      g_free (host);
      return FALSE;
    }

  if (tty_fd == -1)
    res = wait_for_reply (stdout_fd, error);
  else
    res = handle_login (parent, host, port, user, tty_fd, stdout_fd, stderr_fd, error);

  g_strfreev (args);
  g_free (user);
  g_free (host);

  if (!res)
    return FALSE;

  fcntl (stderr_fd, F_SETFL, O_NONBLOCK | fcntl (stderr_fd, F_GETFL));
  is = g_unix_input_stream_new (stderr_fd, FALSE);
  error_stream = g_data_input_stream_new (is);
  g_object_unref (is);
  
  res = look_for_stderr_errors (error_stream, error);
  g_object_unref (error_stream);

  if (!res)
    return FALSE;

  if (tty)
    *tty = tty_fd;

  return TRUE;
}

GQuark 
propono_ssh_error_quark (void)
{
  static GQuark quark = 0;

  if (!quark)
    quark = g_quark_from_string ("propono_ssh_error");

  return quark;
}

/* vim: set ts=8: */
