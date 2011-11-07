#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <glib.h>
#include "CAPSoapHttp.nsmap"
#include "plugin.h"
#include "wsseapi.h"
#include "ipaws.h"
#include "xmpp.h"

EVP_PKEY *rsa_private_key = NULL;
X509 *cert = NULL;
xmpp_ctx_t *ctx;
xmpp_conn_t *conn;
GHashTable* table;
time_t last_update;

struct soap* init_soap(void);
void init_xmpp(void);
void cleanup(struct soap*);

void init_xmpp(void) {
  FILE *fd = NULL;
  char xmpp_password[11] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
  xmpp_log_t *log;
  
  xmpp_initialize();
  
  fd = fopen("secrets_xmpp", "r");
  if (fd) {
    fgets(xmpp_password, 11, fd);
    fclose(fd);
    if (xmpp_password == NULL) {
      perror("Unable to read password for XMPP server.");
      exit(EXIT_FAILURE);
    }
  } else {
    perror("Unable to open secrets_xmpp file");
    exit(EXIT_FAILURE);
  }

  /* create a context */
  log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* pass NULL instead to silence output */
  ctx = xmpp_ctx_new(NULL, NULL);

  /* create a connection */
  conn = xmpp_conn_new(ctx);

  /* setup authentication information */
  xmpp_conn_set_jid(conn, "admin@xmpp.stormee.org");
  xmpp_conn_set_pass(conn, xmpp_password);

}

struct soap* init_soap(void) {
  FILE *fd = NULL;
  struct soap* soap = NULL;
  char passwd[10] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

  soap = soap_new1(SOAP_XML_CANONICAL | SOAP_XML_INDENT);
  soap_ssl_init();
  //  soap_register_plugin(soap, plugin); // register plugin
  soap_register_plugin(soap, soap_wsse);

  fd = fopen("secrets", "r");
  if (fd) {
    fgets(passwd, 10, fd);
    fclose(fd);
    if (passwd == NULL) {
      perror("Unable to read password for X509 certificate.");
      return NULL;
    }
  } else {
    perror("Unable to open secrets file");
    return NULL;
  }

  fd = fopen("DMOPEN_100014_PRIVATE.pem", "r");
  if (fd) {
    rsa_private_key = PEM_read_PrivateKey(fd, NULL, NULL, passwd);
    fclose(fd);
    if (rsa_private_key == NULL) {
      perror("Error reading private key");
      return NULL;
    }
  } else {
    perror("Unable to open Private X509 .pem file");
    return NULL;
  }

  fd = fopen("DMOPEN_100014.pem", "r");
  if (fd) {
    cert = PEM_read_X509(fd, NULL, NULL, NULL);
    fclose(fd);
    if (cert == NULL) {
      perror("Error reading certificate file");
      return NULL;
    }
  } else {
    perror("Unable to open publix X509 .pem file");
    return NULL;
  }

  return soap;
}

void cleanup(struct soap* soap) {
  /* release our connection and context */
  xmpp_conn_release(conn);
  xmpp_ctx_free(ctx);
  
  /* final shutdown of the library */
  xmpp_shutdown();

  if (rsa_private_key) {
    EVP_PKEY_free(rsa_private_key);
  }

  if (cert) {
    X509_free(cert);
  }

  if (soap) {
    soap_end(soap);
    soap_done(soap);
    soap_free(soap);
  }
  
  ERR_remove_state(0);
  ENGINE_cleanup();
  CONF_modules_unload(1);

  ERR_free_strings();
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
}

void termination_handler (int signum __attribute__((unused)))
{
  if (conn)
    xmpp_disconnect(conn);
}

int handle_ipaws(xmpp_conn_t * const conn __attribute__((unused)),
                 void * const userdata );

int handle_runfirst(xmpp_conn_t * const conn, void * const userdata) {
  handle_ipaws(conn, userdata);
  return 0;
}

int handle_ipaws(xmpp_conn_t * const conn __attribute__((unused)), 
		 void * const userdata ) {
  struct soap* soap = (struct soap*) userdata;
  static char tmpstr[32] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

  time_t* tmp_timet = NULL;
  time_t derp = last_update - 130;
  strftime(tmpstr,32,"%Y-%m-%dT%H:%M:%S",gmtime(&derp));
  sprintf(tmpstr + strlen(tmpstr), "%+03d:%02d", 0,0);
  derp = time(NULL);

  printf("Fetching since %s\n", tmpstr);
  messages_t* msgs = getMessages(soap, tmpstr);

  if (msgs) {
    last_update = derp;
    for ( unsigned int i = 0; i < msgs->size; i++ ) {
      if ( strncmp(msgs->ids[i], "NOMESSAGEFOUND", 14) == 0 )
	continue;
      if ( g_hash_table_lookup_extended(table, msgs->ids[i], NULL, NULL) )
	continue;
      sendAlert(conn, ctx, msgs->alerts[i], msgs->ids[i]);
      tmp_timet = malloc(sizeof(time_t));
      *tmp_timet = *(msgs->expires[i]);
      g_hash_table_insert(table, strdup(msgs->ids[i]), tmp_timet);
      tmp_timet = NULL;      
    }

    for ( unsigned int i = 0; i < msgs->size; i++ ) {
      free(msgs->alerts[i]);
      free(msgs->ids[i]);
      free(msgs->expires[i]);
    }
    free(msgs->ids);
    free(msgs->alerts);
    free(msgs);
  }

  soap_free_temp(soap);

  return 1;
}

void key_destroy_func(gpointer data) {
  // this is a char*
  free(data);
}

void value_destroy_func(gpointer data) {
  // this is a time_t*
  free(data);
}

int main(void) {
  struct soap* soap = init_soap();
  struct sigaction new_action, old_action;

  init_xmpp();

  table = g_hash_table_new(g_str_hash, g_str_equal);
  last_update = time(NULL) - 60*60*24*3;

  /* Set up the structure to specify the new action. */
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  
  sigaction (SIGINT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGINT, &new_action, NULL);
  sigaction (SIGHUP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);
  sigaction (SIGTERM, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGTERM, &new_action, NULL);


  xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

  if (soap) {
    xmpp_timed_handler_add(conn, handle_ipaws, 60000, soap);
    xmpp_timed_handler_add(conn, handle_runfirst, 1000, soap);
    xmpp_run(ctx);

    //printRespList(doPing(soap));
    //printRespList(getServerInfo(soap));
    //printRespList(getCOGList(soap));
    //printRespList(getValueListURN(soap, "https://www.dmopen.fema.gov/RequestOperationList"));
    //printRespList(getMessageList(soap, "2011-10-18T15:21:00-00:00"));
    //getMessages(soap, "2011-10-20T22:21:00-00:00");
  } else {
    perror("Error in initialization");
    cleanup(soap);
    return EXIT_FAILURE;
  }

  g_hash_table_destroy(table);
  cleanup(soap);
  return EXIT_SUCCESS;
}
