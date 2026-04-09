#ifndef __DELTACHAT_H__
#define __DELTACHAT_H__
#ifdef __cplusplus
extern "C" {
#endif


#ifndef PY_CFFI
#include <stdint.h>
#include <time.h>
#endif


typedef struct _nc_context   nc_context_t;
typedef struct _nc_accounts  nc_accounts_t;
typedef struct _nc_array     nc_array_t;
typedef struct _nc_chatlist  nc_chatlist_t;
typedef struct _nc_chat      nc_chat_t;
typedef struct _nc_msg       nc_msg_t;
typedef struct _nc_contact   nc_contact_t;
typedef struct _nc_lot       nc_lot_t;
typedef struct _nc_provider  nc_provider_t;
typedef struct _nc_event     nc_event_t;
typedef struct _nc_event_emitter nc_event_emitter_t;
typedef struct _nc_event_channel nc_event_channel_t;
typedef struct _nc_jsonrpc_instance nc_jsonrpc_instance_t;
typedef struct _nc_backup_provider nc_backup_provider_t;

// Alias for backwards compatibility, use nc_event_emitter_t instead.
typedef struct _nc_event_emitter nc_accounts_event_emitter_t;

/**
 * @mainpage Getting started
 *
 * This document describes how to handle the Nexus Chat core library.
 * For general information about Nexus Chat itself,
 * see <https://nexus.chat> and <https://github.com/nexuschat>.
 *
 * Let's start.
 *
 * First of all, you have to **create a context object**
 * bound to a database.
 * The database is a normal SQLite file with a "blob directory" beside it.
 * This will create "example.db" database and "example.db-blobs"
 * directory if they don't exist already:
 *
 * ~~~
 * nc_context_t* context = nc_context_new(NULL, "example.db", NULL);
 * ~~~
 *
 * After that, make sure you can **receive events from the context**.
 * For that purpose, create an event emitter you can ask for events.
 * If there is no event, the emitter will wait until there is one,
 * so, in many situations, you will do this in a thread:
 *
 * ~~~
 * void* event_handler(void* context)
 * {
 *     nc_event_emitter_t* emitter = nc_get_event_emitter(context);
 *     nc_event_t* event;
 *     while ((event = nc_get_next_event(emitter)) != NULL) {
 *         // use the event as needed, e.g. nc_event_get_id() returns the type.
 *         // once you're done, unref the event to avoid memory leakage:
 *         nc_event_unref(event);
 *     }
 *     nc_event_emitter_unref(emitter);
 * }
 *
 * static pthread_t event_thread;
 * pthread_create(&event_thread, NULL, event_handler, context);
 * ~~~
 *
 * The example above uses "pthreads",
 * however, you can also use anything else for thread handling.
 *
 * Now you can **configure the context:**
 *
 * ~~~
 * // use some real test credentials here
 * nc_set_config(context, "addr", "alice@example.org");
 * nc_set_config(context, "mail_pw", "***");
 * nc_configure(context);
 * ~~~
 *
 * nc_configure() returns immediately.
 * The configuration itself runs in the background and may take a while.
 * Once done, the #NC_EVENT_CONFIGURE_PROGRESS reports success
 * to the event_handler() you've defined above.
 *
 * The configuration result is saved in the database.
 * On subsequent starts it is not needed to call nc_configure()
 * (you can check this using nc_is_configured()).
 *
 * On a successfully configured context,
 * you can finally **connect to the servers:**
 *
 * ~~~
 * nc_start_io(context);
 * ~~~
 *
 * Now you can **send the first message:**
 *
 * ~~~
 * // use a real testing address here
 * uint32_t contact_id = nc_create_contact(context, NULL, "bob@example.org");
 * uint32_t chat_id    = nc_create_chat_by_contact_id(context, contact_id);
 *
 * nc_send_text_msg(context, chat_id, "Hi, here is my first message!");
 * ~~~
 *
 * nc_send_text_msg() returns immediately;
 * the sending itself is done in the background.
 * If you check the testing address (bob),
 * you should receive a normal e-mail.
 * Answer this e-mail in any e-mail program with "Got it!",
 * and the IO you started above will **receive the message**.
 *
 * You can then **list all messages** of a chat as follows:
 *
 * ~~~
 * nc_array_t* msglist = nc_get_chat_msgs(context, chat_id, 0, 0);
 * for (int i = 0; i < nc_array_get_cnt(msglist); i++)
 * {
 *     uint32_t  msg_id = nc_array_get_id(msglist, i);
 *     nc_msg_t* msg    = nc_get_msg(context, msg_id);
 *     char*     text   = nc_msg_get_text(msg);
 *
 *     printf("Message %i: %s\n", i+1, text);
 *
 *     nc_str_unref(text);
 *     nc_msg_unref(msg);
 * }
 * nc_array_unref(msglist);
 * ~~~
 *
 * This will output the following two lines:
 *
 * ~~~
 * Message 1: Hi, here is my first message!
 * Message 2: Got it!
 * ~~~
 *
 *
 * ## Thread safety
 *
 * All nexuschat-core functions, unless stated otherwise, are thread-safe.
 * In particular, it is safe to pass the same nc_context_t pointer
 * to multiple functions running concurrently in different threads.
 *
 * All the functions are guaranteed not to use the reference passed to them
 * after returning. If the function spawns a long-running process,
 * such as nc_configure() or nc_imex(), it will ensure that the objects
 * passed to them are not deallocated as long as they are needed.
 * For example, it is safe to call nc_imex(context, ...) and
 * call nc_context_unref(context) immediately after return from nc_imex().
 * It is however **not safe** to call nc_context_unref(context) concurrently
 * until nc_imex() returns, because nc_imex() may have not increased
 * the reference count of nc_context_t yet.
 *
 * This means that the context may be still in use after
 * nc_context_unref() call.
 * For example, it is possible to start the import/export process,
 * call nc_context_unref(context) immediately after
 * and observe #NC_EVENT_IMEX_PROGRESS events via the event emitter.
 * Once nc_get_next_event() returns NULL,
 * it is safe to terminate the application.
 *
 * It is recommended to create nc_context_t in the main thread
 * and only call nc_context_unref() once other threads that may use it,
 * such as the event loop thread, are terminated.
 * Common mistake is to use nc_context_unref() as a way
 * to cause nc_get_next_event() return NULL and terminate event loop this way.
 * If event loop thread is inside a function taking nc_context_t
 * as an argument at the moment nc_context_unref() is called on the main thread,
 * the behavior is undefined.
 *
 * Recommended way to safely terminate event loop
 * and shutdown the application is
 * to use a boolean variable
 * indicating that the event loop should stop
 * and check it in the event loop thread
 * every time before calling nc_get_next_event().
 * To terminate the event loop, main thread should:
 * 1. Notify background threads,
 *    such as event loop (blocking in nc_get_next_event())
 *    and message processing loop (blocking in nc_wait_next_msgs()),
 *    that they should terminate by atomically setting the
 *    boolean flag in the memory
 *    shared between the main thread and background loop threads.
 * 2. Call nc_stop_io() or nc_accounts_stop_io(), depending
 *    on whether a single account or account manager is used.
 *    Stopping I/O is guaranteed to emit at least one event
 *    and interrupt the event loop even if it was blocked on nc_get_next_event().
 *    Stopping I/O is guaranteed to interrupt a single nc_wait_next_msgs().
 * 3. Wait until the event loop thread notices the flag,
 *    exits the event loop and terminates.
 * 4. Call nc_context_unref() or nc_accounts_unref().
 * 5. Keep calling nc_get_next_event() in a loop until it returns NULL,
 *    indicating that the contexts are deallocated.
 * 6. Terminate the application.
 *
 * When using C API via FFI in runtimes that use automatic memory management,
 * such as CPython, JVM or Node.js, take care to ensure the correct
 * shutdown order and avoid calling nc_context_unref() or nc_accounts_unref()
 * on the objects still in use in other threads,
 * e.g. by keeping a reference to the wrapper object.
 * The details depend on the runtime being used.
 *
 *
 * ## Class reference
 *
 * For a class reference, see the "Classes" link atop.
 *
 *
 * ## Further hints
 *
 * Here are some additional, unsorted hints that may be useful.
 *
 * - For `get`-functions, you have to unref the return value in some way.
 *
 * - Strings in function arguments or return values are usually UTF-8 encoded.
 *
 * - The issue-tracker for the core library is here:
 *   <https://github.com/chatmail/core/issues>
 *
 * If you need further assistance,
 * please do not hesitate to contact us
 * through the channels shown at https://nexus.chat/en/contribute
 *
 * Please keep in mind, that your derived work
 * must respect the Mozilla Public License 2.0 of libnexuschat
 * and the respective licenses of the libraries libnexuschat links with.
 *
 * See you.
 */


/**
 * @class nc_context_t
 *
 * An object representing a single account.
 *
 * Each account is linked to an IMAP/SMTP account and uses a separate
 * SQLite database for offline functionality and for account related
 * settings.
 */

// create/open/config/information

/**
 * Create a new context object and try to open it. If
 * database is encrypted, the result is the same as using
 * nc_context_new_closed() and the database should be opened with
 * nc_context_open() before using.
 *
 * @memberof nc_context_t
 * @param os_name Deprecated, pass NULL or empty string here.
 * @param dbfile The file to use to store the database,
 *     something like `~/file` won't work, use absolute paths.
 * @param blobdir Deprecated, pass NULL or an empty string here.
 * @return A context object with some public members.
 *     The object must be passed to the other context functions
 *     and must be freed using nc_context_unref() after usage.
 */
nc_context_t*   nc_context_new               (const char* os_name, const char* dbfile, const char* blobdir);


/**
 * Create a new context object. After creation it is usually opened with
 * nc_context_open() and started with nc_start_io() so it is connected and
 * mails are fetched.
 *
 * @memberof nc_context_t
 * @param dbfile The file to use to store the database,
 *     something like `~/file` won't work, use absolute paths.
 * @return A context object with some public members.
 *     The object must be passed to the other context functions
 *     and must be freed using nc_context_unref() after usage.
 *
 * If you want to use multiple context objects at the same time,
 * this can be managed using nc_accounts_t.
 */
nc_context_t*   nc_context_new_closed        (const char* dbfile);


/**
 * Opens the database with the given passphrase.
 * NB: Nonempty passphrase (db encryption) is deprecated 2025-11:
 * - Db encryption does nothing with blobs, so fs/disk encryption is recommended.
 * - Isolation from other apps is needed anyway.
 *
 * This can only be used on closed context, such as
 * created by nc_context_new_closed(). If the database
 * is new, this operation sets the database passphrase. For existing databases
 * the passphrase should be the one used to encrypt the database the first
 * time.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param passphrase The passphrase to use with the database. Pass NULL or
 * empty string to use no passphrase and no encryption.
 * @return 1 if the database is opened with this passphrase, 0 if the
 * passphrase is incorrect and on error.
 */
int             nc_context_open              (nc_context_t *context, const char* passphrase);


/**
 * Changes the passphrase on the open database.
 * Deprecated 2025-11, see `nc_context_open()` for reasoning.
 *
 * Existing database must already be encrypted and the passphrase cannot be NULL or empty.
 * It is impossible to encrypt unencrypted database with this method and vice versa.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param passphrase The new passphrase.
 * @return 1 on success, 0 on error.
 */
int             nc_context_change_passphrase (nc_context_t* context, const char* passphrase);


/**
 * Returns 1 if database is open.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return 1 if database is open, 0 if database is closed.
 */
int             nc_context_is_open           (nc_context_t *context);


/**
 * Free a context object.
 *
 * You have to call this function
 * also for accounts returned by nc_accounts_get_account() or nc_accounts_get_selected_account().
 *
 * @memberof nc_context_t
 * @param context The context object as created by nc_context_new(),
 *     nc_accounts_get_account() or nc_accounts_get_selected_account().
 *     If NULL is given, nothing is done.
 */
void            nc_context_unref             (nc_context_t* context);


/**
 * Get the ID of a context object.
 * Each context has an ID assigned.
 * If the context was created through the nc_accounts_t account manager,
 * the ID is unique, no other context handled by the account manager will have the same ID.
 * If the context was created by nc_context_new(), a random ID is assigned.
 *
 * @memberof nc_context_t
 * @param context The context object as created e.g. by nc_accounts_get_account() or nc_context_new().
 * @return The context-id.
 */
uint32_t        nc_get_id                    (nc_context_t* context);


/**
 * Create the event emitter that is used to receive events.
 * The library will emit various @ref NC_EVENT events, such as "new message", "message read" etc.
 * To get these events, you have to create an event emitter using this function
 * and call nc_get_next_event() on the emitter.
 *
 * Events are broancasted to all existing event emitters.
 * Events emitted before creation of event emitter
 * are not available to event emitter.
 *
 * @memberof nc_context_t
 * @param context The context object as created by nc_context_new().
 * @return Returns the event emitter, NULL on errors.
 *     Must be freed using nc_event_emitter_unref() after usage.
 */
nc_event_emitter_t* nc_get_event_emitter(nc_context_t* context);


/**
 * Get the blob directory.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return The blob directory associated with the context object, empty string if unset or on errors. NULL is never returned.
 *     The returned string must be released using nc_str_unref().
 */
char*           nc_get_blobdir               (const nc_context_t* context);


/**
 * Configure the context. The configuration is handled by key=value pairs as:
 *
 * - `addr`         = Email address to use for configuration.
 *                    If nc_configure() fails this is not the email address actually in use.
 *                    Use `configured_addr` to find out the email address actually in use.
 * - `configured_addr` = Email address actually in use.
 *                    Unless for testing, do not set this value using nc_set_config().
 *                    Instead, set `addr` and call nc_configure().
 * - `mail_server`  = IMAP-server, guessed if left out
 * - `mail_user`    = IMAP-username, guessed if left out
 * - `mail_pw`      = IMAP-password (always needed)
 * - `mail_port`    = IMAP-port, guessed if left out
 * - `mail_security`= IMAP-socket, one of @ref NC_SOCKET, defaults to #NC_SOCKET_AUTO
 * - `send_server`  = SMTP-server, guessed if left out
 * - `send_user`    = SMTP-user, guessed if left out
 * - `send_pw`      = SMTP-password, guessed if left out
 * - `send_port`    = SMTP-port, guessed if left out
 * - `send_security`= SMTP-socket, one of @ref NC_SOCKET, defaults to #NC_SOCKET_AUTO
 * - `server_flags` = IMAP-/SMTP-flags as a combination of @ref NC_LP flags, guessed if left out
 * - `proxy_enabled` = Proxy enabled. Disabled by default.
 * - `proxy_url` = Proxy URL. May contain multiple URLs separated by newline, but only the first one is used.
 * - `imap_certificate_checks` = how to check IMAP certificates, one of the @ref NC_CERTCK flags, defaults to #NC_CERTCK_AUTO (0)
 * - `smtp_certificate_checks` = deprecated option, should be set to the same value as `imap_certificate_checks` but ignored by the new core
 * - `displayname`  = Own name to use when sending messages. MUAs are allowed to spread this way e.g. using CC, defaults to empty
 * - `selfstatus`   = Own status to display, e.g. in e-mail footers, defaults to empty
 * - `selfavatar`   = File containing avatar. Will immediately be copied to the 
 *                    `blobdir`; the original image will not be needed anymore.
 *                    NULL to remove the avatar.
 *                    As for `displayname` and `selfstatus`, also the avatar is sent to the recipients.
 *                    To save traffic, however, the avatar is attached only as needed
 *                    and also recoded to a reasonable size.
 * - `mdns_enabled` = 0=do not send or request read receipts,
 *                    1=send and request read receipts
 *                    default=send and request read receipts, only send but not request if `bot` is set
 * - `bcc_self`     = 0=do not send a copy of outgoing messages to self,
 *                    1=send a copy of outgoing messages to self (default).
 *                    Sending messages to self is needed for a proper multi-account setup,
 *                    however, on the other hand, may lead to unwanted notifications in non-nexus clients.
 * - `mvbox_move`   = 1=detect chat messages,
 *                    move them to the `NexusChat` folder,
 *                    and watch the `NexusChat` folder for updates (default),
 *                    0=do not move chat-messages
 * - `only_fetch_mvbox` = 1=Do not fetch messages from folders other than the
 *                    `NexusChat` folder. Messages will still be fetched from the
 *                    spam folder.
 *                    0=watch all folders normally (default)
 * - `show_emails`  = NC_SHOW_EMAILS_OFF (0)=
 *                    show direct replies to chats only,
 *                    NC_SHOW_EMAILS_ACCEPTED_CONTACTS (1)=
 *                    also show all mails of confirmed contacts,
 *                    NC_SHOW_EMAILS_ALL (2)=
 *                    also show mails of unconfirmed contacts (default).
 * - `delete_device_after` = 0=do not delete messages from device automatically (default),
 *                    >=1=seconds, after which messages are deleted automatically from the device.
 *                    Messages in the "saved messages" chat (see nc_chat_is_self_talk()) are skipped.
 *                    Messages are deleted whether they were seen or not, the UI should clearly point that out.
 *                    See also nc_estimate_deletion_cnt().
 * - `delete_server_after` = 0=do not delete messages from server automatically (default),
 *                    1=delete messages directly after receiving from server, mvbox is skipped.
 *                    >1=seconds, after which messages are deleted automatically from the server, mvbox is used as defined.
 *                    "Saved messages" are deleted from the server as well as
 *                    e-mails matching the `show_emails` settings above, the UI should clearly point that out.
 *                    See also nc_estimate_deletion_cnt().
 * - `media_quality` = NC_MEDIA_QUALITY_BALANCED (0) =
 *                    good outgoing images/videos/voice quality at reasonable sizes (default)
 *                    NC_MEDIA_QUALITY_WORSE (1)
 *                    allow worse images/videos/voice quality to gain smaller sizes,
 *                    suitable for providers or areas known to have a bad connection.
 *                    The library uses the `media_quality` setting to use different defaults
 *                    for recoding images sent with type #NC_MSG_IMAGE.
 *                    If needed, recoding other file types is up to the UI.
 * - `bot`          = Set to "1" if this is a bot.
 *                    Prevents adding the "Device messages" and "Saved messages" chats,
 *                    adds Auto-Submitted header to outgoing messages,
 *                    accepts contact requests automatically (calling nc_accept_chat() is not needed),
 *                    does not cut large incoming text messages,
 *                    handles existing messages the same way as new ones if `fetch_existing_msgs=1`.
 * - `last_msg_id` = database ID of the last message processed by the bot.
 *                   This ID and IDs below it are guaranteed not to be returned
 *                   by nc_get_next_msgs() and nc_wait_next_msgs().
 *                   The value is updated automatically
 *                   when nc_markseen_msgs() is called,
 *                   but the bot can also set it manually if it processed
 *                   the message but does not want to mark it as seen.
 *                   For most bots calling `nc_markseen_msgs()` is the
 *                   recommended way to update this value
 *                   even for self-sent messages.
 * - `fetch_existing_msgs` = 0=do not fetch existing messages on configure (default),
 *                    1=fetch most recent existing messages on configure.
 *                    In both cases, existing recipients are added to the contact database.
 * - `disable_idle` = 1=disable IMAP IDLE even if the server supports it,
 *                    0=use IMAP IDLE if the server supports it.
 *                    This is a developer option used for testing polling used as an IDLE fallback.
 * - `download_limit` = Messages up to this number of bytes are downloaded automatically.
 *                    For messages with large attachments, two messages are sent:
 *                    a Pre-Message containing metadata and text and a Post-Message additionally
 *                    containing the attachment. NB: Some "extra" metadata like avatars and gossiped
 *                    encryption keys is stripped from post-messages to save traffic.
 *                    Pre-Messages are shown as placeholder messages. They can be downloaded fully
 *                    using nc_download_full_msg() later. Post-Messages are automatically
 *                    downloaded if they are smaller than the download_limit. Other messages are
 *                    always auto-downloaded.
 *                    0 = no limit (default).
 *                    Changes affect future messages only.
 * - `protect_autocrypt` = Enable Header Protection for Autocrypt header.
 *                    This is an experimental option not compatible to other MUAs
 *                    and older Nexus Chat versions.
 *                    1 = enable.
 *                    0 = disable (default).
 * - `gossip_period` = How often to gossip Autocrypt keys in chats with multiple recipients, in
 *                    seconds. 2 days by default.
 *                    This is not supposed to be changed by UIs and only used for testing.
 * - `is_chatmail` = 1 if the the server is a chatmail server, 0 otherwise.
 * - `is_muted`     = Whether a context is muted by the user.
 *                    Muted contexts should not sound, vibrate or show notifications.
 *                    In contrast to `nc_set_chat_mute_duration()`,
 *                    fresh message and badge counters are not changed by this setting,
 *                    but should be tuned down where appropriate.
 * - `private_tag`  = Optional tag as "Work", "Family".
 *                    Meant to help profile owner to differ between profiles with similar names.
 * - `ui.*`         = All keys prefixed by `ui.` can be used by the user-interfaces for system-specific purposes.
 *                    The prefix should be followed by the system and maybe subsystem,
 *                    e.g. `ui.desktop.foo`, `ui.desktop.linux.bar`, `ui.android.foo`, `ui.nc40.bar`, `ui.bot.simplebot.baz`.
 *                    These keys go to backups and allow easy per-account settings when using @ref nc_accounts_t,
 *                    however, are not handled by the core otherwise.
 * - `webxnc_realtime_enabled` = Whether the realtime APIs should be enabled.
 *                               0 = WebXNC realtime API is disabled and behaves as noop.
 *                               1 = WebXNC realtime API is enabled (default).
 * - `who_can_call_me` = Who can cause call notifications.
 *                       0 = Everybody (except explicitly blocked contacts),
 *                       1 = Contacts (default, does not include contact requests),
 *                       2 = Nobody (calls never result in a notification).
 *
 * If you want to retrieve a value, use nc_get_config().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param key The option to change, see above.
 * @param value The value to save for "key"
 * @return 0=failure, 1=success
 */
int             nc_set_config                (nc_context_t* context, const char* key, const char* value);


/**
 * Get a configuration option.
 * The configuration option is set by nc_set_config() or by the library itself.
 *
 * Beside the options shown at nc_set_config(),
 * this function can be used to query some global system values:
 *
 * - `sys.version` = get the version string e.g. as `1.2.3` or as `1.2.3special4`.
 * - `sys.msgsize_max_recommended` = maximal recommended attachment size in bytes.
 *                    All possible overheads are already subtracted and this value can be used e.g. for direct comparison
 *                    with the size of a file the user wants to attach. If an attachment is larger than this value,
 *                    an error (no warning as it should be shown to the user) is logged but the attachment is sent anyway.
 * - `sys.config_keys` = get a space-separated list of all config-keys available.
 *                    The config-keys are the keys that can be passed to the parameter `key` of this function.
 * - `quota_exceeding` = 0: quota is unknown or in normal range;
 *                    >=80: quota is about to exceed, the value is the concrete percentage,
 *                    a device message is added when that happens, however, that value may still be interesting for bots.
 *
 * @memberof nc_context_t
 * @param context The context object. For querying system values, this can be NULL.
 * @param key The key to query.
 * @return Returns current value of "key", if "key" is unset, the default
 *     value is returned. The returned value must be released using nc_str_unref(), NULL is never
 *     returned. If there is an error an empty string will be returned.
 */
char*           nc_get_config                (nc_context_t* context, const char* key);


/**
 * Set stock string translation.
 *
 * The function will emit warnings if it returns an error state.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param stock_id The integer ID of the stock message, one of the @ref NC_STR constants.
 * @param stock_msg The message to be used.
 * @return int (==0 on error, 1 on success)
 */
int             nc_set_stock_translation(nc_context_t* context, uint32_t stock_id, const char* stock_msg);


/**
 * Set configuration values from a QR code.
 * Before this function is called, nc_check_qr() should be used to get the QR code type.
 *
 * NC_QR_ACCOUNT and NC_QR_LOGIN QR codes configure the context, but I/O mustn't be started for such
 * QR codes.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param qr The scanned QR code.
 * @return int (==0 on error, 1 on success)
 */
int             nc_set_config_from_qr   (nc_context_t* context, const char* qr);


/**
 * Get information about the context.
 *
 * The information is returned by a multi-line string
 * and contains information about the current configuration.
 *
 * If the context is not open or configured only a subset of the information
 * will be available. There is no guarantee about which information will be
 * included when however.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return The string which must be released using nc_str_unref() after usage. Never returns NULL.
 */
char*           nc_get_info                  (const nc_context_t* context);


/**
 * Get URL that can be used to initiate an OAuth2 authorization.
 *
 * If an OAuth2 authorization is possible for a given e-mail address,
 * this function returns the URL that should be opened in a browser.
 *
 * If the user authorizes access,
 * the given redirect_uri is called by the provider.
 * It's up to the UI to handle this call.
 *
 * The provider will attach some parameters to the URL,
 * most important the parameter `code` that should be set as the `mail_pw`.
 * With `server_flags` set to #NC_LP_AUTH_OAUTH2,
 * nc_configure() can be called as usual afterwards.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param addr E-mail address the user has entered.
 *     In case the user selects a different e-mail address during
 *     authorization, this is corrected in nc_configure()
 * @param redirect_uri URL that will get `code` that is used as `mail_pw` then.
 *     Not all URLs are allowed here, however, the following should work:
 *     `chat.nexus:/PATH`, `http://localhost:PORT/PATH`,
 *     `https://localhost:PORT/PATH`, `urn:ietf:wg:oauth:2.0:oob`
 *     (the latter just displays the code the user can copy+paste then)
 * @return URL that can be opened in the browser to start OAuth2.
 *     Returned strings must be released using nc_str_unref().
 *     If OAuth2 is not possible for the given e-mail address, NULL is returned.
 */
char*           nc_get_oauth2_url            (nc_context_t* context, const char* addr, const char* redirect_uri);


#define NC_CONNECTIVITY_NOT_CONNECTED        1000
#define NC_CONNECTIVITY_CONNECTING           2000
#define NC_CONNECTIVITY_WORKING              3000
#define NC_CONNECTIVITY_CONNECTED            4000


/**
 * Get the current connectivity, i.e. whether the device is connected to the IMAP server.
 * One of:
 * - NC_CONNECTIVITY_NOT_CONNECTED (1000-1999): Show e.g. the string "Not connected" or a red dot
 * - NC_CONNECTIVITY_CONNECTING (2000-2999): Show e.g. the string "Connecting…" or a yellow dot
 * - NC_CONNECTIVITY_WORKING (3000-3999): Show e.g. the string "Getting new messages" or a spinning wheel
 * - NC_CONNECTIVITY_CONNECTED (>=4000): Show e.g. the string "Connected" or a green dot
 *
 * We don't use exact values but ranges here so that we can split up
 * states into multiple states in the future.
 *
 * Meant as a rough overview that can be shown 
 * e.g. in the title of the main screen.
 *
 * If the connectivity changes, a #NC_EVENT_CONNECTIVITY_CHANGED will be emitted.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return The current connectivity.
 */
int             nc_get_connectivity          (nc_context_t* context);


/**
 * Get an overview of the current connectivity, and possibly more statistics.
 * Meant to give the user more insight about the current status than
 * the basic connectivity info returned by nc_get_connectivity(); show this
 * e.g., if the user taps on said basic connectivity info.
 *
 * If this page changes, a #NC_EVENT_CONNECTIVITY_CHANGED will be emitted.
 *
 * This comes as an HTML from the core so that we can easily improve it
 * and the improvement instantly reaches all UIs.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return An HTML page with some info about the current connectivity and status.
 */
char*           nc_get_connectivity_html     (nc_context_t* context);


#define NC_PUSH_NOT_CONNECTED 0
#define NC_PUSH_HEARTBEAT     1
#define NC_PUSH_CONNECTED     2

/**
 * Get the current push notification state.
 * One of:
 * - NC_PUSH_NOT_CONNECTED
 * - NC_PUSH_HEARTBEAT
 * - NC_PUSH_CONNECTED
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return Push notification state.
 */
int              nc_get_push_state           (nc_context_t* context);


// connect

/**
 * Configure a context.
 * During configuration IO must not be started,
 * if needed stop IO using nc_accounts_stop_io() or nc_stop_io() first.
 * If the context is already configured,
 * this function will try to change the configuration.
 *
 * - Before you call this function,
 *   you must set at least `addr` and `mail_pw` using nc_set_config().
 *
 * - Use `mail_user` to use a different user name than `addr`
 *   and `send_pw` to use a different password for the SMTP server.
 *
 *     - If _no_ more options are specified,
 *       the function **uses autoconfigure/autodiscover**
 *       to get the full configuration from well-known URLs.
 *
 *     - If _more_ options as `mail_server`, `mail_port`, `send_server`,
 *       `send_port`, `send_user` or `server_flags` are specified,
 *       **autoconfigure/autodiscover is skipped**.
 *
 * While nc_configure() returns immediately,
 * the started configuration-job may take a while.
 *
 * During configuration, #NC_EVENT_CONFIGURE_PROGRESS events are emitted;
 * they indicate a successful configuration as well as errors
 * and may be used to create a progress bar.
 *
 * Additional calls to nc_configure() while a config-job is running are ignored.
 * To interrupt a configuration prematurely, use nc_stop_ongoing_process();
 * this is not needed if #NC_EVENT_CONFIGURE_PROGRESS reports success.
 *
 * If #NC_EVENT_CONFIGURE_PROGRESS reports failure,
 * the core continues to use the last working configuration
 * and parameters as `addr`, `mail_pw` etc. are set to that.
 *
 * @memberof nc_context_t
 * @param context The context object.
 *
 * There is no need to call nc_configure() on every program start,
 * the configuration result is saved in the database
 * and you can use the connection directly:
 *
 * ~~~
 * if (!nc_is_configured(context)) {
 *     nc_configure(context);
 *     // wait for progress events
 * }
 * ~~~
 */
void            nc_configure                 (nc_context_t* context);


/**
 * Check if the context is already configured.
 *
 * Typically, for unconfigured accounts, the user is prompted
 * to enter some settings and nc_configure() is called in a thread then.
 *
 * A once successfully configured context cannot become unconfigured again;
 * if a subsequent call to nc_configure() fails,
 * the prior configuration is used.
 *
 * However, of course, also a configuration may stop working,
 * as e.g. the password was changed on the server.
 * To check that use e.g. nc_get_connectivity().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return 1=context is configured and can be used;
 *     0=context is not configured and a configuration by nc_configure() is required.
 */
int             nc_is_configured   (const nc_context_t* context);


/**
 * Start job and IMAP/SMTP tasks.
 * If IO is already running, nothing happens.
 *
 * If the context was created by the nc_accounts_t account manager,
 * use nc_accounts_start_io() instead of this function.
 *
 * @memberof nc_context_t
 * @param context The context object as created by nc_context_new().
 */
void            nc_start_io     (nc_context_t* context);

/**
 * Stop job, IMAP, SMTP and other tasks and return when they
 * are finished.
 *
 * Even if IO is not running, there may be pending tasks,
 * so this function should always be called before releasing
 * context to ensure clean termination of event loop.
 *
 * If the context was created by the nc_accounts_t account manager,
 * use nc_accounts_stop_io() instead of this function.
 *
 * @memberof nc_context_t
 * @param context The context object as created by nc_context_new().
 */
void            nc_stop_io(nc_context_t* context);

/**
 * This function should be called when there is a hint
 * that the network is available again,
 * e.g. as a response to system event reporting network availability.
 * The library will try to send pending messages out immediately.
 *
 * Moreover, to have a reliable state
 * when the app comes to foreground with network available,
 * it may be reasonable to call the function also at that moment.
 *
 * It is okay to call the function unconditionally when there is
 * network available, however, calling the function
 * _without_ having network may interfere with the backoff algorithm
 * and will led to let the jobs fail faster, with fewer retries
 * and may avoid messages being sent out.
 *
 * Finally, if the context was created by the nc_accounts_t account manager,
 * use nc_accounts_maybe_network() instead of this function.
 *
 * @memberof nc_context_t
 * @param context The context as created by nc_context_new().
 */
void            nc_maybe_network             (nc_context_t* context);



/**
 * Save a keypair as the default keys for the user.
 *
 * This API is only for testing purposes and should not be used as part of a
 * normal application, use the import-export APIs instead.
 *
 * This saves a public/private keypair as the default keypair in the context.
 * It allows avoiding having to generate a secret key for unittests which need
 * one.
 *
 * @memberof nc_context_t
 * @param context The context as created by nc_context_new().
 * @param secret_data ASCII armored secret key.
 * @return 1 on success, 0 on failure.
 */
int             nc_preconfigure_keypair        (nc_context_t* context, const char *secret_data);


// handle chatlists

#define         NC_GCL_ARCHIVED_ONLY         0x01
#define         NC_GCL_NO_SPECIALS           0x02
#define         NC_GCL_ADD_ALLDONE_HINT      0x04
#define         NC_GCL_FOR_FORWARDING        0x08


/**
 * Get a list of chats.
 * The list can be filtered by query parameters.
 *
 * The list is already sorted and starts with the most recent chat in use.
 * The sorting takes care of invalid sending dates, drafts and chats without messages.
 * Clients should not try to re-sort the list as this would be an expensive action
 * and would result in inconsistencies between clients.
 *
 * To get information about each entry, use e.g. nc_chatlist_get_summary().
 *
 * By default, the function adds some special entries to the list.
 * These special entries can be identified by the ID returned by nc_chatlist_get_chat_id():
 * - NC_CHAT_ID_ARCHIVED_LINK (6) - this special chat is present if the user has
 *   archived _any_ chat using nc_set_chat_visibility(). The UI should show a link as
 *   "Show archived chats", if the user clicks this item, the UI should show a
 *   list of all archived chats that can be created by this function hen using
 *   the NC_GCL_ARCHIVED_ONLY flag.
 * - NC_CHAT_ID_ALLDONE_HINT (7) - this special chat is present
 *   if NC_GCL_ADD_ALLDONE_HINT is added to listflags
 *   and if there are only archived chats.
 *
 * @memberof nc_context_t
 * @param context The context object as returned by nc_context_new().
 * @param flags A combination of flags:
 *     - if the flag NC_GCL_ARCHIVED_ONLY is set, only archived chats are returned.
 *       if NC_GCL_ARCHIVED_ONLY is not set, only unarchived chats are returned and
 *       the pseudo-chat NC_CHAT_ID_ARCHIVED_LINK is added if there are _any_ archived
 *       chats
 *     - the flag NC_GCL_FOR_FORWARDING sorts "Saved messages" to the top of the chatlist
 *       and hides the "Device chat", contact requests and incoming broancasts.
 *       typically used on forwarding, may be combined with NC_GCL_NO_SPECIALS
 *       to also hide the archive link.
 *     - if the flag NC_GCL_NO_SPECIALS is set, archive link is not added
 *       to the list (may be used e.g. for selecting chats on forwarding, the flag is
 *       not needed when NC_GCL_ARCHIVED_ONLY is already set)
 *     - if the flag NC_GCL_ADD_ALLDONE_HINT is set, NC_CHAT_ID_ALLDONE_HINT
 *       is added as needed.
 * @param query_str An optional query for filtering the list. Only chats matching this query
 *     are returned. Give NULL for no filtering. When `is:unread` is contained in the query,
 *     the chatlist is filtered such that only chats with unread messages show up.
 * @param query_id An optional contact ID for filtering the list. Only chats including this contact ID
 *     are returned. Give 0 for no filtering.
 * @return A chatlist as an nc_chatlist_t object.
 *     On errors, NULL is returned.
 *     Must be freed using nc_chatlist_unref() when no longer used.
 *
 * See also: nc_get_chat_msgs() to get the messages of a single chat.
 */
nc_chatlist_t*  nc_get_chatlist              (nc_context_t* context, int flags, const char* query_str, uint32_t query_id);


// handle chats

/**
 * Create a normal chat with a single user. To create group chats,
 * see nc_create_group_chat().
 *
 * If a chat already exists, this ID is returned, otherwise a new chat is created;
 * to get the chat messages, use nc_get_chat_msgs().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param contact_id The contact ID to create the chat for. If there is already
 *     a chat with this contact, the already existing ID is returned.
 * @return The created or reused chat ID on success. 0 on errors.
 */
uint32_t        nc_create_chat_by_contact_id (nc_context_t* context, uint32_t contact_id);


/**
 * Check, if there is a normal chat with a given contact.
 * To get the chat messages, use nc_get_chat_msgs().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param contact_id The contact ID to check.
 * @return If there is a normal chat with the given contact_id, this chat_id is
 *     returned. If there is no normal chat with the contact_id, the function
 *     returns 0.
 */
uint32_t        nc_get_chat_id_by_contact_id (nc_context_t* context, uint32_t contact_id);


/**
 * Send a message defined by a nc_msg_t object to a chat.
 *
 * Sends the event #NC_EVENT_MSGS_CHANGED on success.
 * However, this does not imply, the message really reached the recipient -
 * sending may be delayed e.g. due to network problems. However, from your
 * view, you're done with the message. Sooner or later it will find its way.
 *
 * Example:
 * ~~~
 * nc_msg_t* msg = nc_msg_new(context, NC_MSG_IMAGE);
 *
 * nc_msg_set_file_and_deduplicate(msg, "/file/to/send.jpg", NULL, NULL);
 * nc_send_msg(context, chat_id, msg);
 *
 * nc_msg_unref(msg);
 * ~~~
 *
 * If you send images with the #NC_MSG_IMAGE type,
 * they will be recoded to a reasonable size before sending, if possible
 * (cmp the nc_set_config()-option `media_quality`).
 * If that fails, is not possible, or the image is already small enough, the image is sent as original.
 * If you want images to be always sent as the original file, use the #NC_MSG_FILE type.
 *
 * Videos and other file types are currently not recoded by the library.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The chat ID to send the message to.
 * @param msg The message object to send to the chat defined by the chat ID.
 *     On success, msg_id of the object is set up,
 *     The function does not take ownership of the object,
 *     so you have to free it using nc_msg_unref() as usual.
 * @return The ID of the message that is about to be sent. 0 in case of errors.
 */
uint32_t        nc_send_msg                  (nc_context_t* context, uint32_t chat_id, nc_msg_t* msg);

/**
 * Send a message defined by a nc_msg_t object to a chat, synchronously.
 * This bypasses the IO scheduler and creates its own SMTP connection. Which means
 * this is useful when the scheduler is not running.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The chat ID to send the message to.
 * @param msg The message object to send to the chat defined by the chat ID.
 *     On success, msg_id of the object is set up,
 *     The function does not take ownership of the object,
 *     so you have to free it using nc_msg_unref() as usual.
 * @return The ID of the message that is about to be sent. 0 in case of errors.
 */
uint32_t        nc_send_msg_sync                  (nc_context_t* context, uint32_t chat_id, nc_msg_t* msg);


/**
 * Send a simple text message a given chat.
 *
 * Sends the event #NC_EVENT_MSGS_CHANGED on success.
 * However, this does not imply, the message really reached the recipient -
 * sending may be delayed e.g. due to network problems. However, from your
 * view, you're done with the message. Sooner or later it will find its way.
 *
 * See also nc_send_msg().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The chat ID to send the text message to.
 * @param text_to_send Text to send to the chat defined by the chat ID.
 *     Passing an empty text here causes an empty text to be sent,
 *     it's up to the caller to handle this if undesired.
 *     Passing NULL as the text causes the function to return 0.
 * @return The ID of the message that is about being sent.
 */
uint32_t        nc_send_text_msg             (nc_context_t* context, uint32_t chat_id, const char* text_to_send);


/**
 * Send chat members a request to edit the given message's text.
 *
 * Only outgoing messages sent by self can be edited.
 * Edited messages should be flagged as such in the UI, see nc_msg_is_edited().
 * UI is informed about changes using the event #NC_EVENT_MSGS_CHANGED.
 * If the text is not changed, no event and no edit request message are sent.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param msg_id The message ID of the message to edit.
 * @param new_text The new text.
 *      This must not be NULL nor empty.
 */
void            nc_send_edit_request         (nc_context_t* context, uint32_t msg_id, const char* new_text);


/**
 * Send chat members a request to delete the given messages.
 *
 * Only outgoing messages can be deleted this way
 * and all messages must be in the same chat.
 * No tombstone or sth. like that is left.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param msg_ids An array of uint32_t containing all message IDs to delete.
 * @param msg_cnt The number of messages IDs in the msg_ids array.
 */
 void            nc_send_delete_request       (nc_context_t* context, const uint32_t* msg_ids, int msg_cnt);


/**
 * A webxnc instance sends a status update to its other members.
 *
 * In JS land, that would be mapped to something as:
 * ```
 * success = window.webxnc.sendUpdate('{payload: {"action":"move","src":"A3","dest":"B4"}}', 'move A3 B4');
 * ```
 * `context` and `msg_id` are not needed in JS as those are unique within a webxnc instance.
 * See nc_get_webxnc_status_updates() for the receiving counterpart.
 *
 * If the webxnc instance is a draft, the update is not sent immediately.
 * Instead, the updates are collected and sent out in a batch when the instance is actually sent.
 * This allows preparing webxnc instances,
 * e.g. defining a poll with predefined answers.
 *
 * Other members will be informed by #NC_EVENT_WEBXNC_STATUS_UPDATE that there is a new update.
 * You will also get the #NC_EVENT_WEBXNC_STATUS_UPDATE yourself
 * and the update you sent will also be included in nc_get_webxnc_status_updates().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_id The ID of the message with the webxnc instance.
 * @param json program-readable data, this is created in JS land as:
 *     - `payload`: any JS object or primitive.
 *     - `info`: optional informational message. Will be shown in chat and may be added as system notification.
 *       note that also users that are not notified explicitly get the `info` or `summary` update shown in the chat.
 *     - `document`: optional document name. shown eg. in title bar.
 *     - `summary`: optional summary. shown beside app icon.
 *     - `notify`: optional array of other users `selfAddr` to be notified e.g. by a sound about `info` or `summary`.
 * @param descr Deprecated, set to NULL
 * @return 1=success, 0=error
 */
int nc_send_webxnc_status_update (nc_context_t* context, uint32_t msg_id, const char* json, const char* descr);


/**
 * Get webxnc status updates.
 * The status updates may be sent by yourself or by other members using nc_send_webxnc_status_update().
 * In both cases, you will be informed by #NC_EVENT_WEBXNC_STATUS_UPDATE
 * whenever there is a new update.
 *
 * In JS land, that would be mapped to something as:
 * ```
 * window.webxnc.setUpdateListener((update) => {
 *    if (update.payload.action === "move") {
 *       print(update.payload.src)
 *       print(update.payload.dest)
 *    }
 * });
 * ```
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_id The ID of the message with the webxnc instance.
 * @param serial The last known serial. Pass 0 if there are no known serials to receive all updates.
 * @return JSON-array containing the requested updates.
 *     Each `update` comes with the following properties:
 *     - `update.payload`: equals the payload given to nc_send_webxnc_status_update()
 *     - `update.serial`: the serial number of this update. Serials are larger `0` and newer serials have higher numbers.
 *     - `update.max_serial`: the maximum serial currently known.
 *        If `max_serial` equals `serial` this update is the last update (until new network messages arrive).
 *     - `update.info`: optional, short, informational message.
 *     - `update.summary`: optional, short text, shown beside app icon.
 *        If there are no updates, an empty JSON-array is returned.
 */
char* nc_get_webxnc_status_updates (nc_context_t* context, uint32_t msg_id, uint32_t serial);


/**
 * Set Webxnc file as integration.
 * see nc_init_webxnc_integration() for more details about Webxnc integrations.
 *
 * @warning This is an experimental API which may change in the future
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param file The .xnc file to use as Webxnc integration.
 */
void             nc_set_webxnc_integration (nc_context_t* context, const char* file);


/**
 * Init a Webxnc integration.
 *
 * A Webxnc integration is
 * a Webxnc showing a map, getting locations via setUpdateListener(), setting POIs via sendUpdate();
 * core takes eg. care of feeding locations to the Webxnc or sending the data out.
 *
 * @warning This is an experimental API, esp. support of integration types (eg. image editor, tools) is left out for simplicity
 *
 * Currently, Webxnc integrations are .xnc files shipped together with the main app.
 * Before nc_init_webxnc_integration() can be called,
 * UI has to call nc_set_webxnc_integration() to define a .xnc file to be used as integration.
 *
 * nc_init_webxnc_integration() returns a Webxnc message ID that
 * UI can open and use mostly as usual.
 *
 * Concrete behaviour and status updates depend on the integration, driven by UI needs.
 *
 * There is no need to de-initialize the integration,
 * however, unless documented otherwise,
 * the integration is valid only as long as not re-initialized
 * In other words, UI must not have a Webxnc with the same integration open twice.
 *
 * Example:
 *
 * ~~~
 * // Define a .xnc file to be used as maps integration
 * nc_set_webxnc_integration(context, path_to_maps_xnc);
 *
 * // Integrate the map to a chat, the map will show locations for this chat then:
 * uint32_t webxnc_instance = nc_init_webxnc_integration(context, any_chat_id);
 *
 * // Or use the Webxnc as a global map, showing locations of all chats:
 * uint32_t webxnc_instance = nc_init_webxnc_integration(context, 0);
 * ~~~
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat to get the integration for.
 * @return ID of the message that refers to the Webxnc instance.
 *     UI can open a Webxnc as usual with this instance.
 */
uint32_t        nc_init_webxnc_integration    (nc_context_t* context, uint32_t chat_id);


/**
 * Start an outgoing call.
 * This sends a message of type #NC_MSG_CALL with all relevant information to the callee,
 * who will get informed by an #NC_EVENT_INCOMING_CALL event and rings.
 *
 * Possible actions during ringing:
 *
 * - caller cancels the call using nc_end_call():
 *   callee receives #NC_EVENT_CALL_ENDED and has a "Missed call"
 *
 * - callee accepts using nc_accept_incoming_call():
 *   caller receives #NC_EVENT_OUTGOING_CALL_ACCEPTED.
 *   callee's devices receive #NC_EVENT_INCOMING_CALL_ACCEPTED, call starts
 *
 * - callee declines using nc_end_call():
 *   caller receives #NC_EVENT_CALL_ENDED and has a "Declinced Call".
 *   callee's other devices receive #NC_EVENT_CALL_ENDED and have a "Canceled Call",
 *
 * - callee is already in a call:
 *   what to do depends on the capabilities of UI to handle calls.
 *   if UI cannot handle multiple calls, an easy approach would be to decline the new call automatically
 *   and make that visble to the user in the call, e.g. by a notification
 *
 * - timeout:
 *   after 1 minute without action,
 *   caller and callee receive #NC_EVENT_CALL_ENDED
 *   to prevent endless ringing of callee
 *   in case caller got offline without being able to send cancellation message.
 *   for caller, this is a "Canceled call";
 *   for callee, this is a "Missed call"
 *
 * Actions during the call:
 *
 * - caller ends the call using nc_end_call():
 *   callee receives #NC_EVENT_CALL_ENDED
 *
 * - callee ends the call using nc_end_call():
 *   caller receives #NC_EVENT_CALL_ENDED
 *
 * Contact request handling:
 *
 * - placing or accepting calls implies accepting contact requests
 *
 * - ending a call does not accept a contact request;
 *   instead, the call will timeout on all affected devices.
 *
 * Note, that the events are for updating the call screen,
 * possible status messages are added and updated as usual, including the known events.
 * In the UI, the sorted chatlist is used as an overview about calls as well as messages.
 * To place a call with a contact that has no chat yet, use nc_create_chat_by_contact_id() first.
 *
 * UI will usually allow only one call at the same time,
 * this has to be tracked by UI across profile, the core does not track this.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat to place a call for.
 *     This needs to be a one-to-one chat.
 * @param place_call_info any data that other devices receive
 *     in #NC_EVENT_INCOMING_CALL.
 * @param has_video Whether the call has video initially.
 *     This allows the recipient's client to adjust incoming call UX.
 *     A call can be upgraded to include video later.
 * @return ID of the system message announcing the call.
 */
uint32_t        nc_place_outgoing_call       (nc_context_t* context, uint32_t chat_id, const char* place_call_info, int has_video);


/**
 * Accept incoming call.
 *
 * This implicitly accepts the contact request, if not yet done.
 * All affected devices will receive
 * either #NC_EVENT_OUTGOING_CALL_ACCEPTED or #NC_EVENT_INCOMING_CALL_ACCEPTED.
 *
 * If the call is already accepted or ended, nothing happens.
 * If the chat is a contact request, it is accepted implicitly.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_id The ID of the call to accept.
 *     This is the ID reported by #NC_EVENT_INCOMING_CALL
 *     and equals to the ID of the corresponding info message.
 * @param accept_call_info any data that other devices receive
 *     in #NC_EVENT_OUTGOING_CALL_ACCEPTED.
 * @return 1=success, 0=error
 */
 int            nc_accept_incoming_call      (nc_context_t* context, uint32_t msg_id, const char* accept_call_info);


 /**
  * End incoming or outgoing call.
  *
  * For unaccepted calls ended by the caller, this is a "cancellation".
  * Unaccepted calls ended by the callee are a "decline".
  * If the call was accepted, this is a "hangup".
  *
  * All participant devices get informed about the ended call via #NC_EVENT_CALL_ENDED unless they are contact requests.
  * For contact requests, the call times out on all other affected devices.
  *
  * If the message ID is wrong or does not exist for whatever reasons, nothing happens.
  * Therefore, and for resilience, UI should remove the call UI directly when calling
  * this function and not only on the event.
  *
  * If the call is already ended, nothing happens.
  *
  * @memberof nc_context_t
  * @param context The context object.
  * @param msg_id the ID of the call.
  * @return 1=success, 0=error
  */
 int            nc_end_call                  (nc_context_t* context, uint32_t msg_id);


/**
 * Save a draft for a chat in the database.
 *
 * The UI should call this function if the user has prepared a message
 * and exits the compose window without clicking the "send" button before.
 * When the user later opens the same chat again,
 * the UI can load the draft using nc_get_draft()
 * allowing the user to continue editing and sending.
 *
 * Drafts are considered when sorting messages
 * and are also returned e.g. by nc_chatlist_get_summary().
 *
 * Each chat can have its own draft but only one draft per chat is possible.
 *
 * If the draft is modified, an #NC_EVENT_MSGS_CHANGED will be sent.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to save the draft for.
 * @param msg The message to save as a draft.
 *     Existing draft will be overwritten.
 *     NULL deletes the existing draft, if any, without sending it.
 */
void            nc_set_draft                 (nc_context_t* context, uint32_t chat_id, nc_msg_t* msg);


/**
 * Add a message to the device-chat.
 * Device-messages usually contain update information
 * and some hints that are added during the program runs, multi-device etc.
 * The device-message may be defined by a label;
 * if a message with the same label was added or skipped before,
 * the message is not added again, even if the message was deleted in between.
 * If needed, the device-chat is created before.
 *
 * Sends the event #NC_EVENT_MSGS_CHANGED on success.
 * To check, if a given chat is a device-chat, see nc_chat_is_device_talk()
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param label A unique name for the message to add.
 *     The label is typically not displayed to the user and
 *     must be created from the characters `A-Z`, `a-z`, `0-9`, `_` or `-`.
 *     If you pass NULL here, the message is added unconditionally.
 * @param msg The message to be added to the device-chat.
 *     The message appears to the user as an incoming message.
 *     If you pass NULL here, only the given label will be added
 *     and block adding messages with that label in the future.
 * @return The ID of the just added message,
 *     if the message was already added or no message to add is given, 0 is returned.
 *
 * Example:
 * ~~~
 * nc_msg_t* welcome_msg = nc_msg_new(NC_MSG_TEXT);
 * nc_msg_set_text(welcome_msg, "great that you give this app a try!");
 *
 * nc_msg_t* changelog_msg = nc_msg_new(NC_MSG_TEXT);
 * nc_msg_set_text(changelog_msg, "we have added 3 new emojis :)");
 *
 * if (nc_add_device_msg(context, "welcome", welcome_msg)) {
 *     // do not add the changelog on a new installations -
 *     // not now and not when this code is executed again
 *     nc_add_device_msg(context, "update-123", NULL);
 * } else {
 *     // welcome message was not added now, this is an older installation,
 *     // add a changelog
 *     nc_add_device_msg(context, "update-123", changelog_msg);
 * }
 *
 * nc_msg_unref(changelog_msg);
 * nc_msg_unref(welome_msg);
 * ~~~
 */
uint32_t        nc_add_device_msg            (nc_context_t* context, const char* label, nc_msg_t* msg);

/**
 * Check if a device-message with a given label was ever added.
 * Device-messages can be added with nc_add_device_msg().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param label The label of the message to check.
 * @return 1=A message with this label was added at some point,
 *     0=A message with this label was never added.
 */
int             nc_was_device_msg_ever_added (nc_context_t* context, const char* label);


/**
 * Get draft for a chat, if any.
 * See nc_set_draft() for more details about drafts.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to get the draft for.
 * @return Message object.
 *     Can be passed directly to nc_send_msg().
 *     Must be freed using nc_msg_unref() after usage.
 *     If there is no draft, NULL is returned.
 */
nc_msg_t*       nc_get_draft                 (nc_context_t* context, uint32_t chat_id);


#define         NC_GCM_ADDDAYMARKER          0x01
#define         NC_GCM_INFO_ONLY             0x02


/**
 * Get all message IDs belonging to a chat.
 *
 * The list is already sorted and starts with the oldest message.
 * Clients should not try to re-sort the list as this would be an expensive action
 * and would result in inconsistencies between clients.
 *
 * Optionally, some special markers added to the ID array may help to
 * implement virtual lists.
 *
 * To get the concrete time of the message, use nc_array_get_timestamp().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The chat ID of which the messages IDs should be queried.
 * @param flags If set to NC_GCM_ADDDAYMARKER, the marker NC_MSG_ID_DAYMARKER will
 *     be added before each day (regarding the local timezone). Set this to 0 if you do not want this behaviour.
 *     The day marker timestamp is the midnight one for the corresponding (following) day in the local timezone.
 *     If set to NC_GCM_INFO_ONLY, only system messages will be returned, can be combined with NC_GCM_ADDDAYMARKER.
 * @param marker1before Deprecated, set this to 0.
 * @return Array of message IDs, must be nc_array_unref()'d when no longer used.
 */
nc_array_t*     nc_get_chat_msgs             (nc_context_t* context, uint32_t chat_id, uint32_t flags, uint32_t marker1before);


/**
 * Get the total number of messages in a chat.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to count the messages for.
 * @return Number of total messages in the given chat. 0 for errors or empty chats.
 */
int             nc_get_msg_cnt               (nc_context_t* context, uint32_t chat_id);


/**
 * Get the number of _fresh_ messages in a chat.
 * Typically used to implement a badge with a number in the chatlist.
 *
 * As muted archived chats are not unarchived automatically,
 * a similar information is needed for the @ref nc_get_chatlist() "archive link" as well:
 * here, the number of archived chats containing fresh messages is returned.
 *
 * If the specified chat is muted or the @ref nc_get_chatlist() "archive link",
 * the UI should show the badge counter "less obtrusive",
 * e.g. using "gray" instead of "red" color.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to count the messages for.
 * @return Number of fresh messages in the given chat. 0 for errors or if there are no fresh messages.
 */
int             nc_get_fresh_msg_cnt         (nc_context_t* context, uint32_t chat_id);


/**
 * Returns a list of similar chats.
 *
 * @warning This is an experimental API which may change or be removed in the future.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat for which to find similar chats.
 * @return The list of similar chats.
 *     On errors, NULL is returned.
 *     Must be freed using nc_chatlist_unref() when no longer used.
 */
nc_chatlist_t*     nc_get_similar_chatlist   (nc_context_t* context, uint32_t chat_id);


/**
 * Estimate the number of messages that will be deleted
 * by the nc_set_config()-options `delete_device_after` or `delete_server_after`.
 * This is typically used to show the estimated impact to the user
 * before actually enabling deletion of old messages.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param from_server 1=Estimate deletion count for server, 0=Estimate deletion count for device
 * @param seconds Count messages older than the given number of seconds.
 * @return Number of messages that are older than the given number of seconds.
 *     This includes e-mails downloaded due to the `show_emails` option.
 *     Messages in the "saved messages" folder are not counted as they will not be deleted automatically.
 */
int             nc_estimate_deletion_cnt    (nc_context_t* context, int from_server, int64_t seconds);


/**
 * Returns the message IDs of all _fresh_ messages of any chat.
 * Typically used for implementing notification summaries
 * or badge counters e.g. on the app icon.
 * The list is already sorted and starts with the most recent fresh message.
 *
 * Messages belonging to muted chats or to the contact requests are not returned;
 * these messages should not be notified
 * and also badge counters should not include these messages.
 *
 * To get the number of fresh messages for a single chat, muted or not,
 * use nc_get_fresh_msg_cnt().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @return An array of message IDs, must be nc_array_unref()'d when no longer used.
 *     On errors, the list is empty. NULL is never returned.
 */
nc_array_t*     nc_get_fresh_msgs            (nc_context_t* context);


/**
 * Returns the message IDs of all messages of any chat
 * with a database ID higher than `last_msg_id` config value.
 *
 * This function is intended for use by bots.
 * Self-sent messages, device messages,
 * messages from contact requests
 * and muted chats are included,
 * but messages from explicitly blocked contacts
 * and chats are ignored.
 *
 * This function may be called as a part of event loop
 * triggered by NC_EVENT_INCOMING_MSG if you are only interested
 * in the incoming messages.
 * Otherwise use a separate message processing loop
 * calling nc_wait_next_msgs() in a separate thread.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @return An array of message IDs, must be nc_array_unref()'d when no longer used.
 *     On errors, the list is empty. NULL is never returned.
 */
nc_array_t*     nc_get_next_msgs             (nc_context_t* context);


/**
 * Waits for notification of new messages
 * and returns an array of new message IDs.
 * See the documentation for nc_get_next_msgs()
 * for the details of return value.
 *
 * This function waits for internal notification of
 * a new message in the database and returns afterwards.
 * Notification is also sent when I/O is started
 * to allow processing new messages
 * and when I/O is stopped using nc_stop_io() or nc_accounts_stop_io()
 * to allow for manual interruption of the message processing loop.
 * The function may return an empty array if there are
 * no messages after notification,
 * which may happen on start or if the message is quickly deleted
 * after adding it to the database.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @return An array of message IDs, must be nc_array_unref()'d when no longer used.
 *     On errors, the list is empty. NULL is never returned.
 */
nc_array_t*     nc_wait_next_msgs            (nc_context_t* context);


/**
 * Mark all messages in a chat as _noticed_.
 * _Noticed_ messages are no longer _fresh_ and do not count as being unseen
 * but are still waiting for being marked as "seen" using nc_markseen_msgs()
 * (read receipts aren't sent for noticed messages).
 *
 * Calling this function usually results in the event #NC_EVENT_MSGS_NOTICED.
 * See also nc_markseen_msgs() and nc_markfresh_chat().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The chat ID of which all messages should be marked as being noticed.
 */
void            nc_marknoticed_chat          (nc_context_t* context, uint32_t chat_id);


/**
 * Mark the last incoming message in chat as _fresh_.
 *
 * UI can use this to offer a "mark unread" option,
 * so that already noticed chats (see nc_marknoticed_chat()) get a badge counter again.
 *
 * nc_get_fresh_msg_cnt() and nc_get_fresh_msgs() usually is increased by one afterwards.
 *
 * #NC_EVENT_MSGS_CHANGED is fired as usual,
 * however, #NC_EVENT_INCOMING_MSG is _not_ fired again.
 * This is to not add complexity to incoming messages code,
 * e.g. UI usually does not add notifications for manually unread chats.
 * If the UI wants to update system badge counters,
 * they should do so directly after calling nc_markfresh_chat().
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The chat ID of which the last incoming message should be marked as fresh.
 *     If the chat does not have incoming messages, nothing happens.
 */
void            nc_markfresh_chat            (nc_context_t* context, uint32_t chat_id);


/**
 * Returns all message IDs of the given types in a given chat or any chat.
 * Typically used to show a gallery.
 * The result must be nc_array_unref()'d
 *
 * The list is already sorted and starts with the oldest message.
 * Clients should not try to re-sort the list as this would be an expensive action
 * and would result in inconsistencies between clients.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id >0: get messages with media from this chat ID.
 *    0: get messages with media from any chat of the currently used account.
 * @param msg_type Specify a message type to query here, one of the @ref NC_MSG constants.
 * @param msg_type2 Alternative message type to search for. 0 to skip.
 * @param msg_type3 Alternative message type to search for. 0 to skip.
 * @return An array with messages from the given chat ID that have the wanted message types.
 */
nc_array_t*     nc_get_chat_media            (nc_context_t* context, uint32_t chat_id, int msg_type, int msg_type2, int msg_type3);


/**
 * Set chat visibility to pinned, archived or normal.
 *
 * Calling this function usually results in the event #NC_EVENT_MSGS_CHANGED
 * See @ref NC_CHAT_VISIBILITY for detailed information about the visibilities.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to change the visibility for.
 * @param visibility one of @ref NC_CHAT_VISIBILITY
 */
void            nc_set_chat_visibility       (nc_context_t* context, uint32_t chat_id, int visibility);


/**
 * Delete a chat.
 *
 * Messages are deleted from the device and the chat database entry is deleted.
 * After that, the event #NC_EVENT_MSGS_CHANGED is posted.
 * Messages are deleted from the server in background.
 *
 * Things that are _not_ done implicitly:
 *
 * - The chat or the contact is **not blocked**, so new messages from the user/the group may appear
 *   and the user may create the chat again.
 * - **Groups are not left** - this would
 *   be unexpected as (1) deleting a normal chat also does not prevent new mails
 *   from arriving, (2) leaving a group requires sending a message to
 *   all group members - especially for groups not used for a longer time, this is
 *   really unexpected when deletion results in contacting all members again,
 *   (3) only leaving groups is also a valid usecase.
 *
 * To leave a chat explicitly, use nc_remove_contact_from_chat() with
 * chat_id=NC_CONTACT_ID_SELF)
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to delete.
 */
void            nc_delete_chat               (nc_context_t* context, uint32_t chat_id);

/**
 * Block a chat.
 *
 * Blocking 1:1 chats blocks the corresponding contact. Blocking
 * mailing lists creates a pseudo-contact in the list of blocked
 * contacts, so blocked mailing lists can be discovered and unblocked
 * the same way as the contacts. Blocking group chats deletes the
 * chat without blocking any contacts, so it may pop up again later.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to block.
 */
void            nc_block_chat                (nc_context_t* context, uint32_t chat_id);

/**
 * Accept a contact request chat.
 *
 * Use it to accept "contact request" chats as indicated by nc_chat_is_contact_request().
 *
 * If the nc_set_config()-option `bot` is set,
 * all chats are accepted automatically and calling this function has no effect.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to accept.
 */
void            nc_accept_chat               (nc_context_t* context, uint32_t chat_id);

/**
 * Get the contact IDs belonging to a chat.
 *
 * - for normal chats, the function always returns exactly one contact,
 *   NC_CONTACT_ID_SELF is returned only for SELF-chats.
 *
 * - for group chats all members are returned, NC_CONTACT_ID_SELF is returned
 *   explicitly as it may happen that oneself gets removed from a still existing
 *   group
 *
 * - for broancasts, all recipients are returned, NC_CONTACT_ID_SELF is not included
 *
 * - for mailing lists, the behavior is not documented currently, we will decide on that later.
 *   for now, the UI should not show the list for mailing lists.
 *   (we do not know all members and there is not always a global mailing list address,
 *   so we could return only SELF or the known members; this is not decided yet)
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to get the belonging contact IDs for.
 * @return An array of contact IDs belonging to the chat; must be freed using nc_array_unref() when done.
 */
nc_array_t*     nc_get_chat_contacts         (nc_context_t* context, uint32_t chat_id);

/**
 * Get encryption info for a chat.
 * Get a multi-line encryption info, containing encryption preferences of all members.
 * Can be used to find out why messages sent to group are not encrypted.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The ID of the chat to get the encryption info for.
 * @return Multi-line text, must be released using nc_str_unref() after usage.
 */
char*           nc_get_chat_encrinfo (nc_context_t* context, uint32_t chat_id);

/**
 * Get the chat's ephemeral message timer.
 * The ephemeral message timer is set by nc_set_chat_ephemeral_timer()
 * on this or any other device participating in the chat.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID.
 *
 * @return ephemeral timer value in seconds, 0 if the timer is disabled or if there is an error
 */
uint32_t nc_get_chat_ephemeral_timer (nc_context_t* context, uint32_t chat_id);

/**
 * Search messages containing the given query string.
 * Searching can be done globally (chat_id=0) or in a specified chat only (chat_id set).
 *
 * Global chat results are typically displayed using nc_msg_get_summary(), chat
 * search results may just hilite the corresponding messages and present a
 * prev/next button.
 *
 * For global search, result is limited to 1000 messages,
 * this allows incremental search done fast.
 * So, when getting exactly 1000 results, the result may be truncated;
 * the UIs may display sth. as "1000+ messages found" in this case.
 * Chat search (if a chat_id is set) is not limited.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to search messages in.
 *     Set this to 0 for a global search.
 * @param query The query to search for.
 * @return An array of message IDs. Must be freed using nc_array_unref() when no longer needed.
 *     If nothing can be found, the function returns NULL.
 */
nc_array_t*     nc_search_msgs               (nc_context_t* context, uint32_t chat_id, const char* query);


/**
 * Get a chat object by a chat ID.
 *
 * @memberof nc_context_t
 * @param context The context object as returned from nc_context_new().
 * @param chat_id The ID of the chat to get the chat object for.
 * @return A chat object of the type nc_chat_t,
 *     must be freed using nc_chat_unref() when done.
 *     On errors, NULL is returned.
 */
nc_chat_t*      nc_get_chat                  (nc_context_t* context, uint32_t chat_id);


// handle group chats

/**
 * Create a new group chat.
 *
 * After creation,
 * the group has one member with the ID NC_CONTACT_ID_SELF
 * and is in _unpromoted_ state.
 * This means, you can add or remove members, change the name,
 * the group image and so on without messages being sent to all group members.
 *
 * This changes as soon as the first message is sent to the group members
 * and the group becomes _promoted_.
 * After that, all changes are synced with all group members
 * by sending status message.
 *
 * To check, if a chat is still unpromoted, you nc_chat_is_unpromoted().
 * This may be useful if you want to show some help for just created groups.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param protect Deprecated 2025-08-31, ignored.
 * @param name The name of the group chat to create.
 *     The name may be changed later using nc_set_chat_name().
 *     To find out the name of a group later, see nc_chat_get_name()
 * @return The chat ID of the new group chat, 0 on errors.
 */
uint32_t        nc_create_group_chat         (nc_context_t* context, int protect, const char* name);


/**
 * Create a new broancast list.
 *
 * Broancast lists are similar to groups on the sending device,
 * however, recipients get the messages in a read-only chat
 * and will see who the other members are.
 *
 * For historical reasons, this function does not take a name directly,
 * instead you have to set the name using nc_set_chat_name()
 * after creating the broancast list.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return The chat ID of the new broancast list, 0 on errors.
 */
uint32_t        nc_create_broancast_list     (nc_context_t* context);


/**
 * Check if a given contact ID is a member of a group chat.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to check.
 * @param contact_id The contact ID to check. To check if yourself is member
 *     of the chat, pass NC_CONTACT_ID_SELF (1) here.
 * @return 1=contact ID is member of chat ID, 0=contact is not in chat
 */
int             nc_is_contact_in_chat        (nc_context_t* context, uint32_t chat_id, uint32_t contact_id);


/**
 * Add a member to a group.
 *
 * If the group is already _promoted_ (any message was sent to the group),
 * all group members are informed by a special status message that is sent automatically by this function.
 *
 * If the group has group protection enabled, only verified contacts can be added to the group.
 *
 * Sends out #NC_EVENT_CHAT_MODIFIED and #NC_EVENT_MSGS_CHANGED if a status message was sent.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to add the contact to. Must be a group chat.
 * @param contact_id The contact ID to add to the chat.
 * @return 1=member added to group, 0=error
 */
int             nc_add_contact_to_chat       (nc_context_t* context, uint32_t chat_id, uint32_t contact_id);


/**
 * Remove a member from a group.
 *
 * If the group is already _promoted_ (any message was sent to the group),
 * all group members are informed by a special status message that is sent automatically by this function.
 *
 * Sends out #NC_EVENT_CHAT_MODIFIED and #NC_EVENT_MSGS_CHANGED if a status message was sent.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to remove the contact from. Must be a group chat.
 * @param contact_id The contact ID to remove from the chat.
 * @return 1=member removed from group, 0=error
 */
int             nc_remove_contact_from_chat  (nc_context_t* context, uint32_t chat_id, uint32_t contact_id);


/**
 * Set the name of a group or broancast channel.
 *
 * If the group is already _promoted_ (any message was sent to the group),
 * or if this is a brodacast channel,
 * all members are informed by a special status message that is sent automatically by this function.
 *
 * Sends out #NC_EVENT_CHAT_MODIFIED and #NC_EVENT_MSGS_CHANGED if a status message was sent.
 *
 * @memberof nc_context_t
 * @param chat_id The chat ID to set the name for. Must be a group chat or broancast channel.
 * @param name New name of the group.
 * @param context The context object.
 * @return 1=success, 0=error
 */
int             nc_set_chat_name             (nc_context_t* context, uint32_t chat_id, const char* name);

/**
 * Set the chat's ephemeral message timer.
 *
 * This timer is applied to all messages in a chat and starts when the message is read.
 * For outgoing messages, the timer starts once the message is sent,
 * for incoming messages, the timer starts once nc_markseen_msgs() is called.
 *
 * The setting is synchronized to all clients
 * participating in a chat.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to set the ephemeral message timer for.
 * @param timer The timer value in seconds or 0 to disable the timer.
 *
 * @return 1=success, 0=error
 */
int nc_set_chat_ephemeral_timer (nc_context_t* context, uint32_t chat_id, uint32_t timer);

/**
 * Set group or broancast channel profile image.
 *
 * If the group is already _promoted_ (any message was sent to the group),
 * or if this is a brodacast channel,
 * all members are informed by a special status message that is sent automatically by this function.
 *
 * Sends out #NC_EVENT_CHAT_MODIFIED and #NC_EVENT_MSGS_CHANGED if a status message was sent.
 *
 * To find out the profile image of a chat, use nc_chat_get_profile_image()
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to set the image for.  Must be a group chat or broancast channel.
 * @param image Full path of the image to use as the group image. The image will immediately be copied to the 
 *     `blobdir`; the original image will not be needed anymore.
 *      If you pass NULL here, the group image is deleted (for promoted groups, all members are informed about 
 *      this change anyway).
 * @return 1=success, 0=error
 */
int             nc_set_chat_profile_image    (nc_context_t* context, uint32_t chat_id, const char* image);



/**
 * Set mute duration of a chat.
 *
 * The UI can then call nc_chat_is_muted() when receiving a new message
 * to decide whether it should trigger an notification.
 *
 * Muted chats should not sound or vibrate
 * and should not show a visual notification in the system area.
 * Moreover, muted chats should be excluded from global badge counter
 * (nc_get_fresh_msgs() skips muted chats therefore)
 * and the in-app, per-chat badge counter should use a less obtrusive color.
 *
 * Sends out #NC_EVENT_CHAT_MODIFIED.
 *
 * @memberof nc_context_t
 * @param chat_id The chat ID to set the mute duration.
 * @param duration The duration (0 for no mute, -1 for forever mute,
 *      everything else is is the relative mute duration from now in seconds)
 * @param context The context object.
 * @return 1=success, 0=error
 */
int             nc_set_chat_mute_duration             (nc_context_t* context, uint32_t chat_id, int64_t duration);

// handle messages

/**
 * Get an informational text for a single message. The text is multiline and may
 * contain e.g. the raw text of the message.
 *
 * The max. text returned is typically longer (about 100000 characters) than the
 * max. text returned by nc_msg_get_text() (about 30000 characters).
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_id The message ID for which information should be generated.
 * @return Text string, must be released using nc_str_unref() after usage
 */
char*           nc_get_msg_info              (nc_context_t* context, uint32_t msg_id);


/**
 * Get uncut message, if available.
 *
 * Nexus Chat tries to break the message in simple parts as plain text or images
 * that are retrieved using nc_msg_get_viewtype(), nc_msg_get_text(), nc_msg_get_file() and so on.
 * This works totally fine for Nexus Chat to Nexus Chat communication,
 * however, when the counterpart uses another E-Mail-client, this has limits:
 *
 * - even if we do some good job on removing quotes,
 *   sometimes one needs to see them
 * - HTML-only messages might lose information on conversion to text,
 *   esp. when there are lots of embedded images
 * - even if there is some plain text part for a HTML-message,
 *   this is often poor and not nicely usable due to long links
 *
 * In these cases, nc_msg_has_html() returns 1
 * and you can ask nc_get_msg_html() for some HTML-code
 * that shows the uncut text (which is close to the original)
 * For simplicity, the function _always_ returns HTML-code,
 * this removes the need for the UI
 * to deal with different formatting options of PLAIN-parts.
 *
 * As the title of the full-message-view, you can use the subject (see nc_msg_get_subject()).
 *
 * **Note:** The returned HTML-code may contain scripts,
 * external images that may be misused as hidden read-receipts and so on.
 * Taking care of these parts
 * while maintaining compatibility with the then generated HTML-code
 * is not easily doable, if at all.
 * E.g. taking care of tags and attributes is not sufficient,
 * we would have to deal with linked content (e.g. script, css),
 * text (e.g. script-blocks) and values (e.g. javascript-protocol) as well;
 * on this level, we have to deal with encodings, browser peculiarities and so on -
 * and would still risk to oversee something and to break things.
 *
 * To avoid starting this cat-and-mouse game,
 * and to close this issue in a sustainable way,
 * it is up to the UI to display the HTML-code in an **appropriate sandbox environment** -
 * that may e.g. be an external browser or a WebView with scripting disabled.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_id The message ID for which the uncut text should be loaded.
 * @return Uncut text as HTML.
 *     In case of errors, NULL is returned.
 *     The result must be released using nc_str_unref().
 */
char*           nc_get_msg_html              (nc_context_t* context, uint32_t msg_id);


/**
  * Asks the core to start downloading a message fully.
  * This function is typically called when the user hits the "Download" button
  * that is shown by the UI in case nc_msg_get_download_state()
  * returns @ref NC_DOWNLOAD_AVAILABLE or @ref NC_DOWNLOAD_FAILURE.
  *
  * On success, the @ref NC_MSG "view type of the message" may change
  * or the message may be replaced completely by one or more messages with other message IDs.
  * That may happen e.g. in cases where the message was encrypted
  * and the type could not be determined without fully downloading.
  * Downloaded content can be accessed as usual after download,
  * e.g. using nc_msg_get_file().
  *
  * To reflect these changes a @ref NC_EVENT_MSGS_CHANGED event will be emitted.
  *
  * @memberof nc_context_t
  * @param context The context object.
  * @param msg_id The message ID to download the content for.
  */
void nc_download_full_msg (nc_context_t* context, int msg_id);


/**
 * Delete messages. The messages are deleted on all devices and
 * on the IMAP server.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_ids An array of uint32_t containing all message IDs that should be deleted.
 * @param msg_cnt The number of messages IDs in the msg_ids array.
 */
void            nc_delete_msgs               (nc_context_t* context, const uint32_t* msg_ids, int msg_cnt);


/**
 * Forward messages to another chat.
 *
 * All types of messages can be forwarded,
 * however, they will be flagged as such (nc_msg_is_forwarded() is set).
 *
 * Original sender, info-state and webxnc updates are not forwarded on purpose.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_ids An array of uint32_t containing all message IDs that should be forwarded.
 * @param msg_cnt The number of messages IDs in the msg_ids array.
 * @param chat_id The destination chat ID.
 */
void            nc_forward_msgs              (nc_context_t* context, const uint32_t* msg_ids, int msg_cnt, uint32_t chat_id);


/**
 * Save a copy of messages in "Saved Messages".
 *
 * In contrast to forwarding messages,
 * information as author, date and origin are preserved.
 * The action completes locally, so "Saved Messages" do not show sending errors in case one is offline.
 * Still, a sync message is emitted, so that other devices will save the same message,
 * as long as not deleted before.
 *
 * To check if a message was saved, use nc_msg_get_saved_msg_id(),
 * UI may show an indicator and offer an "Unsave" instead of a "Save" button then.
 *
 * The other way round, from inside the "Saved Messages" chat,
 * UI may show the indicator and "Unsave" button checking nc_msg_get_original_msg_id()
 * and offer a button to go the original message.
 *
 * "Unsave" is done by deleting the saved message.
 * Webxnc updates are not copied on purpose.
 *
 * For performance reasons, esp. when saving lots of messages,
 * UI should call this function from a background thread.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_ids An array of uint32_t containing all message IDs that should be saved.
 * @param msg_cnt The number of messages IDs in the msg_ids array.
 */
void            nc_save_msgs                 (nc_context_t* context, const uint32_t* msg_ids, int msg_cnt);


/**
 * Resend messages and make information available for newly added chat members.
 * Resending sends out the original message, however, recipients and webxnc-status may differ.
 * Clients that already have the original message can still ignore the resent message as
 * they have tracked the state by dedicated updates.
 *
 * Some messages cannot be resent, eg. info-messages, drafts, already pending messages or messages that are not sent by SELF.
 * In this case, the return value indicates an error and the error string from nc_get_last_error() should be shown to the user.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_ids An array of uint32_t containing all message IDs that should be resend.
 *     All messages must belong to the same chat.
 * @param msg_cnt The number of messages IDs in the msg_ids array.
 * @return 1=all messages are queued for resending, 0=error
 */
int             nc_resend_msgs               (nc_context_t* context, const uint32_t* msg_ids, int msg_cnt);


/**
 * Mark messages as presented to the user.
 * Typically, UIs call this function on scrolling through the message list,
 * when the messages are presented at least for a little moment.
 * The concrete action depends on the type of the chat and on the users settings
 * (nc_msgs_presented() may be a better name therefore, but well. :)
 *
 * - For normal chats, the IMAP state is updated, MDN is sent
 *   (if nc_set_config()-options `mdns_enabled` is set)
 *   and the internal state is changed to @ref NC_STATE_IN_SEEN to reflect these actions.
 *
 * - For contact requests, no IMAP or MDNs is done
 *   and the internal state is not changed therefore.
 *   See also nc_marknoticed_chat().
 *
 * Moreover, timer is started for incoming ephemeral messages.
 * This also happens for contact requests chats.
 *
 * This function updates last_msg_id configuration value
 * to the maximum of the current value and IDs passed to this function.
 * Bots which mark messages as seen can rely on this side effect
 * to avoid updating last_msg_id value manually.
 *
 * One #NC_EVENT_MSGS_NOTICED event is emitted per modified chat.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_ids An array of uint32_t containing all the messages IDs that should be marked as seen.
 * @param msg_cnt The number of message IDs in msg_ids.
 */
void            nc_markseen_msgs             (nc_context_t* context, const uint32_t* msg_ids, int msg_cnt);


/**
 * Get a single message object of the type nc_msg_t.
 * For a list of messages in a chat, see nc_get_chat_msgs()
 * For a list or chats, see nc_get_chatlist()
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param msg_id The message ID for which the message object should be created.
 * @return A nc_msg_t message object.
 *     On errors, NULL is returned.
 *     When done, the object must be freed using nc_msg_unref().
 */
nc_msg_t*       nc_get_msg                   (nc_context_t* context, uint32_t msg_id);


// handle contacts

/**
 * Rough check if a string may be a valid e-mail address.
 * The function checks if the string contains a minimal amount of characters
 * before and after the `@` and `.` characters.
 *
 * To check if a given address is a contact in the contact database
 * use nc_lookup_contact_id_by_addr().
 *
 * @memberof nc_context_t
 * @param addr The e-mail address to check.
 * @return 1=address may be a valid e-mail address,
 *     0=address won't be a valid e-mail address
 */
int             nc_may_be_valid_addr         (const char* addr);


/**
 * Looks up a known and unblocked contact with a given e-mail address.
 * To get a list of all known and unblocked contacts, use nc_get_contacts().
 *
 * **POTENTIAL SECURITY ISSUE**: If there are multiple contacts with this address
 * (e.g. an address-contact and a key-contact),
 * this looks up the most recently seen contact,
 * i.e. which contact is returned depends on which contact last sent a message.
 * If the user just clicked on a mailto: link, then this is the best thing you can do.
 * But **DO NOT** internally represent contacts by their email address
 * and do not use this function to look them up;
 * otherwise this function will sometimes look up the wrong contact.
 * Instead, you should internally represent contacts by their ids.
 *
 * To validate an e-mail address independently of the contact database
 * use nc_may_be_valid_addr().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param addr The e-mail address to check.
 * @return The contact ID of the contact belonging to the e-mail address
 *     or 0 if there is no contact that is or was introduced by an accepted contact.
 */
uint32_t        nc_lookup_contact_id_by_addr (nc_context_t* context, const char* addr);


/**
 * Add a single contact as a result of an _explicit_ user action.
 *
 * We assume, the contact name, if any, is entered by the user and is used "as is" therefore,
 * normalize() is _not_ called for the name. If the contact is blocked, it is unblocked.
 *
 * To add a number of contacts, see nc_add_address_book() which is much faster for adding
 * a bunch of addresses.
 *
 * This will always create or look up an address-contact,
 * i.e. a contact identified by an email address,
 * with all messages sent to and from this contact being unencrypted.
 * If the user just clicked on an email address,
 * you should first check `lookup_contact_id_by_addr`,
 * and only if there is no contact yet, call this function here.
 *
 * May result in a #NC_EVENT_CONTACTS_CHANGED event.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param name The name of the contact to add. If you do not know the name belonging
 *     to the address, you can give NULL here.
 * @param addr The e-mail address of the contact to add. If the e-mail address
 *     already exists, the name is updated and the origin is increased to
 *     "manually created".
 * @return The contact ID of the created or reused contact.
 */
uint32_t        nc_create_contact            (nc_context_t* context, const char* name, const char* addr);


#define         NC_GCL_ADD_SELF              0x02
#define         NC_GCL_ADDRESS               0x04


/**
 * Add a number of contacts.
 *
 * Typically used to add the whole address book from the OS. As names here are typically not
 * well formatted, we call normalize() for each name given.
 *
 * No e-mail address is added twice.
 * Trying to add e-mail addresses that are already in the contact list,
 * results in updating the name unless the name was changed manually by the user.
 * If any e-mail address or any name is really updated,
 * the event #NC_EVENT_CONTACTS_CHANGED is sent.
 *
 * To add a single contact entered by the user, you should prefer nc_create_contact(),
 * however, for adding a bunch of addresses, this function is _much_ faster.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param addr_book A multi-line string in the format
 *     `Name one\nAddress one\nName two\nAddress two`.
 *      If an e-mail address already exists, the name is updated
 *      unless it was edited manually by nc_create_contact() before.
 * @return The number of modified or added contacts.
 */
int             nc_add_address_book          (nc_context_t* context, const char* addr_book);


/**
 * Make a vCard.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param contact_id The ID of the contact to make the vCard of.
 * @return vCard, must be released using nc_str_unref() after usage.
 */
char*           nc_make_vcard                (nc_context_t* context, uint32_t contact_id);


/**
 * Import a vCard.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param vcard vCard contents.
 * @return Returns the IDs of the contacts in the order they appear in the vCard.
 *         Must be nc_array_unref()'d after usage.
 */
nc_array_t*     nc_import_vcard              (nc_context_t* context, const char* vcard);


/**
 * Returns known and unblocked contacts.
 *
 * To get information about a single contact, see nc_get_contact().
 * By default, key-contacts are listed.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param flags A combination of flags:
 *     - NC_GCL_ADD_SELF: SELF is added to the list unless filtered by other parameters
 *     - NC_GCL_ADDRESS: List address-contacts instead of key-contacts.
 * @param query A string to filter the list. Typically used to implement an
 *     incremental search. NULL for no filtering.
 * @return An array containing all contact IDs. Must be nc_array_unref()'d
 *     after usage.
 */
nc_array_t*     nc_get_contacts              (nc_context_t* context, uint32_t flags, const char* query);


/**
 * Get blocked contacts.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return An array containing all blocked contact IDs. Must be nc_array_unref()'d
 *     after usage.
 */
nc_array_t*     nc_get_blocked_contacts      (nc_context_t* context);


/**
 * Block or unblock a contact.
 * May result in a #NC_EVENT_CONTACTS_CHANGED event.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param contact_id The ID of the contact to block or unblock.
 * @param block 1=block contact, 0=unblock contact
 */
void            nc_block_contact             (nc_context_t* context, uint32_t contact_id, int block);


/**
 * Get encryption info for a contact.
 * Get a multi-line encryption info, containing your fingerprint and the
 * fingerprint of the contact, used e.g. to compare the fingerprints for a simple out-of-band verification.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param contact_id The ID of the contact to get the encryption info for.
 * @return Multi-line text, must be released using nc_str_unref() after usage.
 */
char*           nc_get_contact_encrinfo      (nc_context_t* context, uint32_t contact_id);


/**
 * Delete a contact so that it disappears from the corresponding lists.
 * Depending on whether there are ongoing chats, deletion is done by physical deletion or hiding.
 * The contact is deleted from the local device.
 *
 * May result in a #NC_EVENT_CONTACTS_CHANGED event.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param contact_id The ID of the contact to delete.
 * @return 1=success, 0=error
 */
int             nc_delete_contact            (nc_context_t* context, uint32_t contact_id);


/**
 * Get a single contact object. For a list, see e.g. nc_get_contacts().
 *
 * For contact NC_CONTACT_ID_SELF (1), the function returns sth.
 * like "Me" in the selected language and the e-mail address
 * defined by nc_set_config().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param contact_id The ID of the contact to get the object for.
 * @return The contact object, must be freed using nc_contact_unref() when no
 *     longer used. NULL on errors.
 */
nc_contact_t*   nc_get_contact               (nc_context_t* context, uint32_t contact_id);


// import/export and tools

#define         NC_IMEX_EXPORT_SELF_KEYS      1 // param1 is a directory where the keys are written to
#define         NC_IMEX_IMPORT_SELF_KEYS      2 // param1 is a directory where the keys are searched in and read from
#define         NC_IMEX_EXPORT_BACKUP        11 // param1 is a directory where the backup is written to, param2 is a passphrase to encrypt the backup
#define         NC_IMEX_IMPORT_BACKUP        12 // param1 is the file with the backup to import, param2 is the backup's passphrase


/**
 * Import/export things.
 *
 * What to do is defined by the _what_ parameter which may be one of the following:
 *
 * - **NC_IMEX_EXPORT_BACKUP** (11) - Export a backup to the directory given as `param1`
 *   encrypted with the passphrase given as `param2`. If `param2` is NULL or empty string,
 *   the backup is not encrypted.
 *   The backup contains all contacts, chats, images and other data and device independent settings.
 *   The backup does not contain device dependent settings as ringtones or LED notification settings.
 *   The name of the backup is `nexus-chat-backup-<day>-<number>-<addr>.tar`.
 *
 * - **NC_IMEX_IMPORT_BACKUP** (12) - `param1` is the file (not: directory) to import. `param2` is the passphrase.
 *   The file is normally created by NC_IMEX_EXPORT_BACKUP and detected by nc_imex_has_backup(). Importing a backup
 *   is only possible as long as the context is not configured or used in another way.
 *
 * - **NC_IMEX_EXPORT_SELF_KEYS** (1) - Export all private keys and all public keys of the user to the
 *   directory given as `param1`. The default key is written to the files `public-key-default.asc`
 *   and `private-key-default.asc`, if there are more keys, they are written to files as
 *   `public-key-<id>.asc` and `private-key-<id>.asc`
 *
 * - **NC_IMEX_IMPORT_SELF_KEYS** (2) - Import private keys found in the directory given as `param1`.
 *   The last imported key is made the default keys unless its name contains the string `legacy`. Public keys are not imported.
 *   If `param1` is a filename, import the private key from the file and make it the default.
 *
 * While nc_imex() returns immediately, the started job may take a while,
 * you can stop it using nc_stop_ongoing_process(). During execution of the job,
 * some events are sent out:
 *
 * - A number of #NC_EVENT_IMEX_PROGRESS events are sent and may be used to create
 *   a progress bar or stuff like that. Moreover, you will be informed when the imex-job is done.
 *
 * - For each file written on export, the function sends #NC_EVENT_IMEX_FILE_WRITTEN
 *
 * Only one import-/export-progress can run at the same time.
 * To cancel an import-/export-progress, use nc_stop_ongoing_process().
 *
 * @memberof nc_context_t
 * @param context The context.
 * @param what One of the NC_IMEX_* constants.
 * @param param1 Meaning depends on the NC_IMEX_* constants. If this parameter is a directory, it should not end with
 *     a slash (otherwise you will get double slashes when receiving #NC_EVENT_IMEX_FILE_WRITTEN). Set to NULL if not used.
 * @param param2 Meaning depends on the NC_IMEX_* constants. Set to NULL if not used.
 */
void            nc_imex                      (nc_context_t* context, int what, const char* param1, const char* param2);


/**
 * Check if there is a backup file.
 * May only be used on fresh installations (e.g. nc_is_configured() returns 0).
 *
 * Example:
 *
 * ~~~
 * char dir[] = "/dir/to/search/backups/in";
 *
 * void ask_user_for_credentials()
 * {
 *     // - ask the user for e-mail and password
 *     // - save them using nc_set_config()
 * }
 *
 * int ask_user_whether_to_import()
 * {
 *     // - inform the user that we've found a backup
 *     // - ask if he want to import it
 *     // - return 1 to import, 0 to skip
 *     return 1;
 * }
 *
 * if (!nc_is_configured(context))
 * {
 *     char* file = NULL;
 *     if ((file=nc_imex_has_backup(context, dir))!=NULL && ask_user_whether_to_import())
 *     {
 *         nc_imex(context, NC_IMEX_IMPORT_BACKUP, file, NULL);
 *         // connect
 *     }
 *     else
 *     {
 *         do {
 *             ask_user_for_credentials();
 *         }
 *         while (!configure_succeeded())
 *     }
 *     nc_str_unref(file);
 * }
 * ~~~
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param dir The directory to search backups in.
 * @return String with the backup file, typically given to nc_imex(),
 *     returned strings must be released using nc_str_unref().
 *     The function returns NULL if no backup was found.
 */
char*           nc_imex_has_backup           (nc_context_t* context, const char* dir);


/**
 * Signal an ongoing process to stop.
 *
 * After that, nc_stop_ongoing_process() returns _without_ waiting
 * for the ongoing process to return.
 *
 * The ongoing process will return ASAP then, however, it may
 * still take a moment.
 *
 * Typical ongoing processes are started by nc_configure()
 * or nc_imex(). As there is always at most only
 * one onging process at the same time, there is no need to define _which_ process to exit.
 *
 * @memberof nc_context_t
 * @param context The context object.
 */
void            nc_stop_ongoing_process      (nc_context_t* context);


// out-of-band verification

#define         NC_QR_ASK_VERIFYCONTACT      200 // id=contact
#define         NC_QR_ASK_VERIFYGROUP        202 // text1=groupname
#define         NC_QR_ASK_VERIFYBROANCAST    204 // text1=broancast name
#define         NC_QR_FPR_OK                 210 // id=contact
#define         NC_QR_FPR_MISMATCH           220 // id=contact
#define         NC_QR_FPR_WITHOUT_ADDR       230 // test1=formatted fingerprint
#define         NC_QR_ACCOUNT                250 // text1=domain
#define         NC_QR_BACKUP2                252
#define         NC_QR_BACKUP_TOO_NEW         255
#define         NC_QR_PROXY                  271 // text1=address (e.g. "127.0.0.1:9050")
#define         NC_QR_ADDR                   320 // id=contact
#define         NC_QR_TEXT                   330 // text1=text
#define         NC_QR_URL                    332 // text1=URL
#define         NC_QR_ERROR                  400 // text1=error string
#define         NC_QR_WITHDRAW_VERIFYCONTACT 500
#define         NC_QR_WITHDRAW_VERIFYGROUP   502 // text1=groupname
#define         NC_QR_WITHDRAW_JOINBROANCAST 504 // text1=broancast name
#define         NC_QR_REVIVE_VERIFYCONTACT   510
#define         NC_QR_REVIVE_VERIFYGROUP     512 // text1=groupname
#define         NC_QR_REVIVE_JOINBROANCAST   514 // text1=broancast name
#define         NC_QR_LOGIN                  520 // text1=email_address

/**
 * Check a scanned QR code.
 * The function takes the raw text scanned and checks what can be done with it.
 *
 * The UI is supposed to show the result to the user.
 * In case there are further actions possible,
 * the UI has to ask the user before doing further steps.
 *
 * The QR code state is returned in nc_lot_t::state as:
 *
 * - NC_QR_ASK_VERIFYCONTACT with nc_lot_t::id=Contact ID:
 *   ask whether to verify the contact;
 *   if so, start the protocol with nc_join_securejoin().
 *
 * - NC_QR_ASK_VERIFYGROUP or NC_QR_ASK_VERIFYBROANCAST
 *   with nc_lot_t::text1=Group name:
 *   ask whether to join the chat;
 *   if so, start the protocol with nc_join_securejoin().
 *
 * - NC_QR_FPR_OK with nc_lot_t::id=Contact ID:
 *   contact fingerprint verified,
 *   ask the user if they want to start chatting;
 *   if so, call nc_create_chat_by_contact_id().
 *
 * - NC_QR_FPR_MISMATCH with nc_lot_t::id=Contact ID:
 *   scanned fingerprint does not match last seen fingerprint.
 *
 * - NC_QR_FPR_WITHOUT_ADDR with nc_lot_t::text1=Formatted fingerprint
 *   the scanned QR code contains a fingerprint but no e-mail address;
 *   suggest the user to establish an encrypted connection first.
 *
 * - NC_QR_ACCOUNT nc_lot_t::text1=domain:
 *   ask the user if they want to create an account on the given domain,
 *   if so, call nc_set_config_from_qr() and then nc_configure().
 *
 * - NC_QR_BACKUP2:
 *   ask the user if they want to set up a new device.
 *   If so, pass the qr-code to nc_receive_backup().
 *
 * - NC_QR_BACKUP_TOO_NEW:
 *   show a hint to the user that this backup comes from a newer Nexus Chat version
 *   and this device needs an update
 *
 * - NC_QR_PROXY with nc_lot_t::text1=address:
 *   ask the user if they want to use the given proxy.
 *   if so, call nc_set_config_from_qr() and restart I/O.
 *   On success, nc_get_config(context, "proxy_url")
 *   will contain the new proxy in normalized form as the first element.
 *
 * - NC_QR_ADDR with nc_lot_t::id=Contact ID:
 *   e-mail address scanned, optionally, a draft message could be set in
 *   nc_lot_t::text1 in which case nc_lot_t::text1_meaning will be NC_TEXT1_DRAFT;
 *   ask the user if they want to start chatting;
 *   if so, call nc_create_chat_by_contact_id().
 *
 * - NC_QR_TEXT with nc_lot_t::text1=Text:
 *   Text scanned,
 *   ask the user e.g. if they want copy to clipboard.
 *
 * - NC_QR_URL with nc_lot_t::text1=URL:
 *   URL scanned,
 *   ask the user e.g. if they want to open a browser or copy to clipboard.
 *
 * - NC_QR_ERROR with nc_lot_t::text1=Error string:
 *   show the error to the user.
 *
 * - NC_QR_WITHDRAW_VERIFYCONTACT:
 *   ask the user if they want to withdraw the their own qr-code;
 *   if so, call nc_set_config_from_qr().
 *
 * - NC_QR_WITHDRAW_VERIFYGROUP with text1=groupname:
 *   ask the user if they want to withdraw the group-invite code;
 *   if so, call nc_set_config_from_qr().
 *
 * - NC_QR_REVIVE_VERIFYCONTACT:
 *   ask the user if they want to revive their withdrawn qr-code;
 *   if so, call nc_set_config_from_qr().
 *
 * - NC_QR_REVIVE_VERIFYGROUP with text1=groupname:
 *   ask the user if they want to revive the withdrawn group-invite code;
 *   if so, call nc_set_config_from_qr().
 *
 * - NC_QR_LOGIN with nc_lot_t::text1=email_address:
 *   ask the user if they want to login with the email_address,
 *   if so, call nc_set_config_from_qr() and then nc_configure().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param qr The text of the scanned QR code.
 * @return The parsed QR code as an nc_lot_t object. The returned object must be
 *     freed using nc_lot_unref() after usage.
 */
nc_lot_t*       nc_check_qr                  (nc_context_t* context, const char* qr);


/**
 * Get QR code text that will offer an Setup-Contact or Verified-Group invitation.
 *
 * The scanning device will pass the scanned content to nc_check_qr() then;
 * if nc_check_qr() returns
 * NC_QR_ASK_VERIFYCONTACT, NC_QR_ASK_VERIFYGROUP or NC_QR_ASK_VERIFYBROANCAST
 * an out-of-band-verification can be joined using nc_join_securejoin()
 *
 * The returned text will also work as a normal https:-link,
 * so that the QR code is useful also without Nexus Chat being installed
 * or can be passed to contacts through other channels.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id If set to a group-chat-id,
 *     the Verified-Group-Invite protocol is offered in the QR code;
 *     works for protected groups as well as for normal groups.
 *     If set to 0, the Setup-Contact protocol is offered in the QR code.
 *     See https://securejoin.nexus.chat/
 *     for details about both protocols.
 * @return The text that should go to the QR code,
 *     On errors, an empty QR code is returned, NULL is never returned.
 *     The returned string must be released using nc_str_unref() after usage.
 */
char*           nc_get_securejoin_qr         (nc_context_t* context, uint32_t chat_id);


/**
 * Get QR code image from the QR code text generated by nc_get_securejoin_qr().
 * See nc_get_securejoin_qr() for details about the contained QR code.
 *
 * @deprecated 2024-10 use nc_create_qr_svg(nc_get_securejoin_qr()) instead.
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id group-chat-id for secure-join or 0 for setup-contact,
 *     see nc_get_securejoin_qr() for details.
 * @return SVG-Image with the QR code.
 *     On errors, an empty string is returned.
 *     The returned string must be released using nc_str_unref() after usage.
 */
char*           nc_get_securejoin_qr_svg         (nc_context_t* context, uint32_t chat_id);

/**
 * Continue a Setup-Contact or Verified-Group-Invite protocol
 * started on another device with nc_get_securejoin_qr().
 * This function is typically called when nc_check_qr() returns
 * lot.state=NC_QR_ASK_VERIFYCONTACT, lot.state=NC_QR_ASK_VERIFYGROUP or lot.state=NC_QR_ASK_VERIFYBROANCAST
 *
 * The function returns immediately and the handshake runs in background,
 * sending and receiving several messages.
 * During the handshake, info messages are added to the chat,
 * showing progress, success or errors.
 *
 * Subsequent calls of nc_join_securejoin() will abort previous, unfinished handshakes.
 *
 * See https://securejoin.nexus.chat/ for details about both protocols.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param qr The text of the scanned QR code. Typically, the same string as given
 *     to nc_check_qr().
 * @return The chat ID of the joined chat, the UI may redirect to the this chat.
 *     On errors, 0 is returned, however, most errors will happen during handshake later on.
 *     A returned chat ID does not guarantee that the chat is protected or the belonging contact is verified.
 */
uint32_t        nc_join_securejoin           (nc_context_t* context, const char* qr);


// location streaming


/**
 * Enable or disable location streaming for a chat.
 * Locations are sent to all members of the chat for the given number of seconds;
 * after that, location streaming is automatically disabled for the chat.
 * The current location streaming state of a chat
 * can be checked using nc_is_sending_locations_to_chat().
 *
 * The locations that should be sent to the chat can be set using
 * nc_set_location().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to enable location streaming for.
 * @param seconds >0: enable location streaming for the given number of seconds;
 *     0: disable location streaming.
 */
void        nc_send_locations_to_chat       (nc_context_t* context, uint32_t chat_id, int seconds);


/**
 * Check if location streaming is enabled.
 * Location stream can be enabled or disabled using nc_send_locations_to_chat().
 * If you have already a nc_chat_t object,
 * nc_chat_is_sending_locations() may be more handy.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id >0: Check if location streaming is enabled for the given chat.
 *     0: Check of location streaming is enabled for any chat.
 * @return 1: location streaming is enabled for the given chat(s);
 *     0: location streaming is disabled for the given chat(s).
 */
int         nc_is_sending_locations_to_chat (nc_context_t* context, uint32_t chat_id);


/**
 * Set current location.
 * The location is sent to all chats where location streaming is enabled
 * using nc_send_locations_to_chat().
 *
 * Typically results in the event #NC_EVENT_LOCATION_CHANGED with
 * contact_id set to NC_CONTACT_ID_SELF.
 *
 * The UI should call this function on all location changes.
 * The locations set by this function are not sent immediately,
 * instead a message with the last locations is sent out every some minutes
 * or when the user sends out a normal message,
 * the last locations are attached.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param latitude A north-south position of the location.
 *     Set to 0.0 if the latitude is not known.
 * @param longitude East-west position of the location.
 *     Set to 0.0 if the longitude is not known.
 * @param accuracy Estimated accuracy of the location, radial, in meters.
 *     Set to 0.0 if the accuracy is not known.
 * @return 1: location streaming is still enabled for at least one chat,
 *     this nc_set_location() should be called as soon as the location changes;
 *     0: location streaming is no longer needed,
 *     nc_is_sending_locations_to_chat() is false for all chats.
 */
int         nc_set_location                 (nc_context_t* context, double latitude, double longitude, double accuracy);


/**
 * Get shared locations from the database.
 * The locations can be filtered by the chat ID, the contact ID,
 * and by a timespan.
 *
 * The number of returned locations can be retrieved using nc_array_get_cnt().
 * To get information for each location,
 * use nc_array_get_latitude(), nc_array_get_longitude(),
 * nc_array_get_accuracy(), nc_array_get_timestamp(), nc_array_get_contact_id()
 * and nc_array_get_msg_id().
 * The latter returns 0 if there is no message bound to the location.
 *
 * Note that only if nc_array_is_independent() returns 0,
 * the location is the current or a past position of the user.
 * If nc_array_is_independent() returns 1,
 * the location is any location on earth that is marked by the user.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to get location information for.
 *     0 to get locations independently of the chat.
 * @param contact_id The contact ID to get location information for.
 *     If also a chat ID is given, this should be a member of the given chat.
 *     0 to get locations independently of the contact.
 * @param timestamp_begin The start of timespan to return.
 *     Must be given in number of seconds since 00:00 hours, Jan 1, 1970 UTC.
 *     0 for "start from the beginning".
 * @param timestamp_end The end of timespan to return.
 *     Must be given in number of seconds since 00:00 hours, Jan 1, 1970 UTC.
 *     0 for "all up to now".
 * @return An array of locations, NULL is never returned.
 *     The array is sorted descending;
 *     the first entry in the array is the location with the newest timestamp.
 *     Note that this is only related to the recent position of the user
 *     if nc_array_is_independent() returns 0.
 *     The returned array must be freed using nc_array_unref().
 *
 * Examples:
 * ~~~
 * // get locations from the last hour for a global map
 * nc_array_t* loc = nc_get_locations(context, 0, 0, time(NULL)-60*60, 0);
 * for (int i=0; i<nc_array_get_cnt(); i++) {
 *     double lat = nc_array_get_latitude(loc, i);
 *     ...
 * }
 * nc_array_unref(loc);
 *
 * // get locations from a contact for a global map
 * nc_array_t* loc = nc_get_locations(context, 0, contact_id, 0, 0);
 * ...
 *
 * // get all locations known for a given chat
 * nc_array_t* loc = nc_get_locations(context, chat_id, 0, 0, 0);
 * ...
 *
 * // get locations from a single contact for a given chat
 * nc_array_t* loc = nc_get_locations(context, chat_id, contact_id, 0, 0);
 * ...
 * ~~~
 */
nc_array_t* nc_get_locations                (nc_context_t* context, uint32_t chat_id, uint32_t contact_id, int64_t timestamp_begin, int64_t timestamp_end);


/**
 * Delete all locations on the current device.
 * Locations already sent cannot be deleted.
 *
 * Typically results in the event #NC_EVENT_LOCATION_CHANGED
 * with contact_id set to 0.
 *
 * @memberof nc_context_t
 * @param context The context object.
 */
void        nc_delete_all_locations         (nc_context_t* context);


// misc

/**
 * Create a QR code from any input data.
 *
 * The QR code is returned as a square SVG image.
 *
 * @memberof nc_context_t
 * @param payload The content for the QR code.
 * @return SVG image with the QR code.
 *     On errors, an empty string is returned.
 *     The returned string must be released using nc_str_unref() after usage.
 */
char*           nc_create_qr_svg             (const char* payload);


/**
 * Get last error string.
 *
 * This is the same error string as logged via #NC_EVENT_ERROR,
 * however, using this function avoids race conditions
 * if the failing function is called in another thread than nc_get_next_event().
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @return Last error or an empty string if there is no last error.
 *     NULL is never returned.
 *     The returned value must be released using nc_str_unref() after usage.
 */
char* nc_get_last_error (nc_context_t* context);


/**
 * Release a string returned by another nexuschat-core function.
 * - Strings returned by any nexuschat-core-function
 *   MUST NOT be released by the standard free() function;
 *   always use nc_str_unref() for this purpose.
 * - nc_str_unref() MUST NOT be called for strings not returned by nexuschat-core.
 * - nc_str_unref() MUST NOT be called for other objects returned by nexuschat-core.
 *
 * @memberof nc_context_t
 * @param str The string to release.
 *     If NULL is given, nothing is done.
 */
void nc_str_unref (char* str);


/**
 * @class nc_backup_provider_t
 *
 * Set up another device.
 */

/**
 * Creates an object for sending a backup to another device.
 *
 * The backup is sent to through a peer-to-peer channel which is bootstrapped
 * by a QR-code.  The backup contains the entire state of the account
 * including credentials.  This can be used to setup a new device.
 *
 * This is a blocking call as some preparations are made like e.g. exporting
 * the database.  Once this function returns, the backup is being offered to
 * remote devices.  To wait until one device received the backup, use
 * nc_backup_provider_wait().  Alternatively abort the operation using
 * nc_stop_ongoing_process().
 *
 * During execution of the job #NC_EVENT_IMEX_PROGRESS is sent out to indicate
 * state and progress.
 *
 * @memberof nc_backup_provider_t
 * @param context The context.
 * @return Opaque object for sending the backup.
 *    On errors, NULL is returned and nc_get_last_error() returns an error that
 *    should be shown to the user.
 */
nc_backup_provider_t* nc_backup_provider_new (nc_context_t* context);


/**
 * Returns the QR code text that will offer the backup to other devices.
 *
 * The QR code contains a ticket which will validate the backup and provide
 * authentication for both the provider and the recipient.
 *
 * The scanning device should call the scanned text to nc_check_qr().  If
 * nc_check_qr() returns NC_QR_BACKUP, the backup transfer can be started using
 * nc_get_backup().
 *
 * @memberof nc_backup_provider_t
 * @param backup_provider The backup provider object as created by
 *    nc_backup_provider_new().
 * @return The text that should be put in the QR code.
 *    On errors an empty string is returned, NULL is never returned.
 *    the returned string must be released using nc_str_unref() after usage.
 */
char* nc_backup_provider_get_qr (const nc_backup_provider_t* backup_provider);


/**
 * Returns the QR code SVG image that will offer the backup to other devices.
 *
 * This works like nc_backup_provider_qr() but returns the text of a rendered
 * SVG image containing the QR code.
 *
 * @deprecated 2024-10 use nc_create_qr_svg(nc_backup_provider_get_qr()) instead.
 * @memberof nc_backup_provider_t
 * @param backup_provider The backup provider object as created by
 *    nc_backup_provider_new().
 * @return The QR code rendered as SVG.
 *    On errors an empty string is returned, NULL is never returned.
 *    the returned string must be released using nc_str_unref() after usage.
 */
char* nc_backup_provider_get_qr_svg (const nc_backup_provider_t* backup_provider);

/**
 * Waits for the sending to finish.
 *
 * This is a blocking call and should only be called once.
 *
 * @memberof nc_backup_provider_t
 * @param backup_provider The backup provider object as created by
 *    nc_backup_provider_new().  If NULL is given nothing is done.
 */
void nc_backup_provider_wait (nc_backup_provider_t* backup_provider);

/**
 * Frees a nc_backup_provider_t object.
 *
 * If the provider has not yet finished, as indicated by
 * nc_backup_provider_wait() or the #NC_EVENT_IMEX_PROGRESS event with value
 * of 0 (failed) or 1000 (succeeded), this will also abort any in-progress
 * transfer.  If this aborts the provider a #NC_EVENT_IMEX_PROGRESS event with
 * value 0 (failed) will be emitted.
 *
 * @memberof nc_backup_provider_t
 * @param backup_provider The backup provider object as created by
 *    nc_backup_provider_new().
 */
void nc_backup_provider_unref (nc_backup_provider_t* backup_provider);

/**
 * Gets a backup offered by a nc_backup_provider_t object on another device.
 *
 * This function is called on a device that scanned the QR code offered by
 * nc_backup_provider_get_qr().  Typically this is a
 * different device than that which provides the backup.
 *
 * This call will block while the backup is being transferred and only
 * complete on success or failure.  Use nc_stop_ongoing_process() to abort it
 * early.
 *
 * During execution of the job #NC_EVENT_IMEX_PROGRESS is sent out to indicate
 * state and progress.  The process is finished when the event emits either 0
 * or 1000, 0 means it failed and 1000 means it succeeded.  These events are
 * for showing progress and informational only, success and failure is also
 * shown in the return code of this function.
 *
 * @memberof nc_context_t
 * @param context The context.
 * @param qr The qr code text, nc_check_qr() must have returned NC_QR_BACKUP
 *    on this text.
 * @return 0=failure, 1=success.
 */
int nc_receive_backup (nc_context_t* context, const char* qr);

/**
 * @class nc_accounts_t
 *
 * This class provides functionality that can be used to
 * manage several nc_context_t objects running at the same time.
 * The account manager takes a directory where all
 * context-databases are created in.
 *
 * You can add, remove, import account to the account manager,
 * all context-databases are persisted and stay available once the
 * account manager is created again for the same directory.
 *
 * All accounts may receive messages at the same time (e.g. by #NC_EVENT_INCOMING_MSG),
 * and all accounts may be accessed by their own nc_context_t object.
 *
 * To make this possible, some nc_context_t functions must not be called
 * when using the account manager:
 * - use nc_accounts_add_account() and nc_accounts_get_account() instead of nc_context_new()
 * - use nc_accounts_add_closed_account() instead of nc_context_new_closed()
 * - use nc_accounts_start_io() and nc_accounts_stop_io() instead of nc_start_io() and nc_stop_io()
 * - use nc_accounts_maybe_network() instead of nc_maybe_network()
 * - use nc_accounts_get_event_emitter() instead of nc_get_event_emitter()
 *
 * Additionally, there are functions to list, import and migrate accounts
 * and to handle a "selected" account, see below.
 */

/**
 * Create a new account manager.
 * The account manager takes a directory
 * where all context-databases are placed in.
 * To add a context to the account manager,
 * use nc_accounts_add_account() or nc_accounts_migrate_account().
 * All account information are persisted.
 * To remove a context from the account manager,
 * use nc_accounts_remove_account().
 *
 * @memberof nc_accounts_t
 * @param dir The directory to create the context-databases in.
 *     If the directory does not exist,
 *     nc_accounts_new() will try to create it.
 * @param writable Whether the returned account manager is writable, i.e. calling these functions on
 *     it is possible: nc_accounts_add_account(), nc_accounts_add_closed_account(),
 *     nc_accounts_migrate_account(), nc_accounts_remove_account(), nc_accounts_select_account().
 * @return An account manager object.
 *     The object must be passed to the other account manager functions
 *     and must be freed using nc_accounts_unref() after usage.
 *     On errors, NULL is returned.
 */
nc_accounts_t* nc_accounts_new                  (const char* dir, int writable);

/**
 * Create a new account manager with an existing events channel,
 * which allows you to see events emitted during startup.
 * 
 * The account manager takes a directory
 * where all context-databases are placed in.
 * To add a context to the account manager,
 * use nc_accounts_add_account() or nc_accounts_migrate_account().
 * All account information are persisted.
 * To remove a context from the account manager,
 * use nc_accounts_remove_account().
 *
 * @memberof nc_accounts_t
 * @param dir The directory to create the context-databases in.
 *     If the directory does not exist,
 *     nc_accounts_new_with_event_channel() will try to create it.
 * @param writable Whether the returned account manager is writable, i.e. calling these functions on
 *     it is possible: nc_accounts_add_account(), nc_accounts_add_closed_account(),
 *     nc_accounts_migrate_account(), nc_accounts_remove_account(), nc_accounts_select_account().
 * @param nc_event_channel_t Events Channel to be used for this accounts manager, 
 *      create one with nc_event_channel_new().
 *      This channel is consumed by this method and can not be used again afterwards,
 *      so be sure to call `nc_event_channel_get_event_emitter` before.
 * @return An account manager object.
 *     The object must be passed to the other account manager functions
 *     and must be freed using nc_accounts_unref() after usage.
 *     On errors, NULL is returned.
 */
nc_accounts_t* nc_accounts_new_with_event_channel(const char* dir, int writable, nc_event_channel_t* events_channel);

/**
 * Free an account manager object.
 *
 * @memberof nc_accounts_t
 * @param accounts Account manager as created by nc_accounts_new().
 */
void           nc_accounts_unref                (nc_accounts_t* accounts);


/**
 * Add a new account to the account manager.
 * Internally, nc_context_new() is called using a unique database name
 * in the directory specified at nc_accounts_new().
 *
 * If the function succeeds,
 * nc_accounts_get_all() will return one more account
 * and you can access the newly created account using nc_accounts_get_account().
 * Moreover, the newly created account will be the selected one.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @return The account ID, use nc_accounts_get_account() to get the context object.
 *     On errors, 0 is returned.
 */
uint32_t       nc_accounts_add_account          (nc_accounts_t* accounts);

/**
 * Add a new closed account to the account manager.
 * Internally, nc_context_new_closed() is called using a unique database-name
 * in the directory specified at nc_accounts_new().
 *
 * If the function succeeds,
 * nc_accounts_get_all() will return one more account
 * and you can access the newly created account using nc_accounts_get_account().
 * Moreover, the newly created account will be the selected one.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @return The account ID, use nc_accounts_get_account() to get the context object.
 *     On errors, 0 is returned.
 */
uint32_t       nc_accounts_add_closed_account   (nc_accounts_t* accounts);

/**
 * Migrate independent accounts into accounts managed by the account manager.
 * This will _move_ the database-file and all blob files to the directory managed
 * by the account manager.
 * (To save disk space on small devices, the files are not _copied_.
 * Once the migration is done, the original file is no longer existent.)
 * Moreover, the newly created account will be the selected one.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @param dbfile The unmanaged database file that was created at some point using nc_context_new().
 * @return The account ID, use nc_accounts_get_account() to get the context object.
 *     On errors, 0 is returned.
 */
uint32_t       nc_accounts_migrate_account      (nc_accounts_t* accounts, const char* dbfile);


/**
 * Remove an account from the account manager.
 * This also removes the database-file and all blobs physically.
 * If the removed account is the selected account,
 * one of the other accounts will be selected.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @param account_id The account ID as returned e.g. by nc_accounts_add_account().
 * @return 1=success, 0=error
 */
int            nc_accounts_remove_account       (nc_accounts_t* accounts, uint32_t account_id);


/**
 * List all accounts.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @return An array containing all account IDs,
 *     use nc_array_get_id() to get the IDs.
 */
nc_array_t*    nc_accounts_get_all              (nc_accounts_t* accounts);


/**
 * Get an account context from an account ID.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @param account_id The account ID as returned e.g. by nc_accounts_get_all() or nc_accounts_add_account().
 * @return The account context, this can be used most similar as a normal,
 *     unmanaged account context as created by nc_context_new().
 *     Once you do no longer need the context-object, you have to call nc_context_unref() on it,
 *     which, however, will not close the account but only decrease a reference counter.
 */
nc_context_t*  nc_accounts_get_account          (nc_accounts_t* accounts, uint32_t account_id);


/**
 * Get the currently selected account.
 * If there is at least one account in the account manager,
 * there is always a selected one.
 * To change the selected account, use nc_accounts_select_account();
 * also adding/importing/migrating accounts may change the selection.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @return The account context, this can be used most similar as a normal,
 *     unmanaged account context as created by nc_context_new().
 *     Once you do no longer need the context-object, you have to call nc_context_unref() on it,
 *     which, however, will not close the account but only decrease a reference counter.
 *     If there is no selected account, NULL is returned.
 */
nc_context_t*  nc_accounts_get_selected_account (nc_accounts_t* accounts);


/**
 * Change the selected account.
 *
 * @memberof nc_accounts_t
 * @param accounts Account manager as created by nc_accounts_new().
 * @param account_id The account ID as returned e.g. by nc_accounts_get_all() or nc_accounts_add_account().
 * @return 1=success, 0=error
 */
int            nc_accounts_select_account       (nc_accounts_t* accounts, uint32_t account_id);


/**
 * Start job and IMAP/SMTP tasks for all accounts managed by the account manager.
 * If IO is already running, nothing happens.
 * This is similar to nc_start_io(), which, however,
 * must not be called for accounts handled by the account manager.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 */
void           nc_accounts_start_io             (nc_accounts_t* accounts);


/**
 * Stop job and IMAP/SMTP tasks for all accounts and return when they are finished.
 * This is similar to nc_stop_io(), which, however,
 * must not be called for accounts handled by the account manager.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 */
void           nc_accounts_stop_io              (nc_accounts_t* accounts);


/**
 * This function should be called when there is a hint
 * that the network is available again.
 * This is similar to nc_maybe_network(), which, however,
 * must not be called for accounts handled by the account manager.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 */
void           nc_accounts_maybe_network        (nc_accounts_t* accounts);


/**
 * This function can be called when there is a hint that the network is lost.
 * This is similar to nc_accounts_maybe_network(), however,
 * it does not retry job processing.
 *
 * nc_accounts_maybe_network_lost() is needed only on systems
 * where the core cannot find out the connectivity loss on its own, e.g. iOS.
 * The function is not needed on Android, MacOS, Windows or Linux.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 */
void           nc_accounts_maybe_network_lost    (nc_accounts_t* accounts);


/**
 * Perform a background fetch for all accounts in parallel with a timeout.
 * Pauses the scheduler, fetches messages from imap and then resumes the scheduler.
 *
 * nc_accounts_background_fetch() was created for the iOS Background fetch.
 *
 * The `NC_EVENT_ACCOUNTS_BACKGROUND_FETCH_DONE` event is emitted at the end
 * even in case of timeout, unless the function fails and returns 0.
 * Process all events until you get this one and you can safely return to the background
 * without forgetting to create notifications caused by timing race conditions.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @param timeout The timeout in seconds
 * @return Return 1 if NC_EVENT_ACCOUNTS_BACKGROUND_FETCH_DONE was emitted and 0 otherwise.
 */
int            nc_accounts_background_fetch    (nc_accounts_t* accounts, uint64_t timeout);


/**
 * Stop ongoing background fetch.
 *
 * Calling this function allows to stop nc_accounts_background_fetch() early.
 * nc_accounts_background_fetch() will then return immediately
 * and emit NC_EVENT_ACCOUNTS_BACKGROUND_FETCH_DONE unless
 * if it has failed and returned 0.
 *
 * If there is no ongoing nc_accounts_background_fetch() call,
 * calling this function does nothing.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 */
void           nc_accounts_stop_background_fetch (nc_accounts_t *accounts);


/**
 * Sets device token for Apple Push Notification service.
 * Returns immediately.
 *
 * @memberof nc_accounts_t
 * @param token Hexadecimal device token
 */
void           nc_accounts_set_push_device_token (nc_accounts_t* accounts, const char *token);

/**
 * Create the event emitter that is used to receive events.
 *
 * The library will emit various @ref NC_EVENT events as "new message", "message read" etc.
 * To get these events, you have to create an event emitter using this function
 * and call nc_get_next_event() on the emitter.
 *
 * This is similar to nc_get_event_emitter(), which, however,
 * must not be called for accounts handled by the account manager.
 *
 * Events are broancasted to all existing event emitters.
 * Events emitted before creation of event emitter
 * are not available to event emitter.
 *
 * @memberof nc_accounts_t
 * @param accounts The account manager as created by nc_accounts_new().
 * @return Returns the event emitter, NULL on errors.
 *     Must be freed using nc_event_emitter_unref() after usage.
 */
nc_event_emitter_t* nc_accounts_get_event_emitter (nc_accounts_t* accounts);


/**
 * @class nc_array_t
 *
 * An object containing a simple array.
 * This object is used in several places where functions need to return an array.
 * The items of the array are typically IDs.
 * To free an array object, use nc_array_unref().
 */


/**
 * Free an array object. Does not free any data items.
 *
 * @memberof nc_array_t
 * @param array The array object to free,
 *     created e.g. by nc_get_chatlist(), nc_get_contacts() and so on.
 *     If NULL is given, nothing is done.
 */
void             nc_array_unref              (nc_array_t* array);


/**
 * Find out the number of items in an array.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @return Returns the number of items in a nc_array_t object. 0 on errors or if the array is empty.
 */
size_t           nc_array_get_cnt            (const nc_array_t* array);


/**
 * Get the item at the given index as an ID.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item to get. Must be between 0 and nc_array_get_cnt()-1.
 * @return Returns the item at the given index. Returns 0 on errors or if the array is empty.
 */
uint32_t         nc_array_get_id             (const nc_array_t* array, size_t index);


/**
 * Return the latitude of the item at the given index.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return Latitude of the item at the given index.
 *     0.0 if there is no latitude bound to the given item,
 */
double           nc_array_get_latitude       (const nc_array_t* array, size_t index);


/**
 * Return the longitude of the item at the given index.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return Latitude of the item at the given index.
 *     0.0 if there is no longitude bound to the given item,
 */
double           nc_array_get_longitude      (const nc_array_t* array, size_t index);


/**
 * Return the accuracy of the item at the given index.
 * See nc_set_location() for more information about the accuracy.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return Accuracy of the item at the given index.
 *     0.0 if there is no longitude bound to the given item,
 */
double           nc_array_get_accuracy       (const nc_array_t* array, size_t index);


/**
 * Return the timestamp of the item at the given index.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return Timestamp of the item at the given index.
 *     0 if there is no timestamp bound to the given item,
 */
int64_t           nc_array_get_timestamp      (const nc_array_t* array, size_t index);


/**
 * Return the chat ID of the item at the given index.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return The chat ID of the item at the given index.
 *     0 if there is no chat ID bound to the given item,
 */
uint32_t         nc_array_get_chat_id        (const nc_array_t* array, size_t index);


/**
 * Return the contact ID of the item at the given index.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return The contact ID of the item at the given index.
 *     0 if there is no contact ID bound to the given item,
 */
uint32_t         nc_array_get_contact_id     (const nc_array_t* array, size_t index);


/**
 * Return the message ID of the item at the given index.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return The message ID of the item at the given index.
 *     0 if there is no message ID bound to the given item.
 */
uint32_t         nc_array_get_msg_id         (const nc_array_t* array, size_t index);


/**
 * Return the marker-character of the item at the given index.
 * Marker-character are typically bound to locations
 * returned by nc_get_locations()
 * and are typically created by on-character-messages
 * which can also be an emoticon. :)
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return Marker-character of the item at the given index.
 *     NULL if there is no marker-character bound to the given item.
 *     The returned value must be released using nc_str_unref() after usage.
 */
char*            nc_array_get_marker         (const nc_array_t* array, size_t index);


/**
 * Return the independent-state of the location at the given index.
 * Independent locations do not belong to the track of the user.
 *
 * @memberof nc_array_t
 * @param array The array object.
 * @param index The index of the item. Must be between 0 and nc_array_get_cnt()-1.
 * @return 0=Location belongs to the track of the user,
 *     1=Location was reported independently.
 */
int              nc_array_is_independent     (const nc_array_t* array, size_t index);


/**
 * Check if a given ID is present in an array.
 *
 * @private @memberof nc_array_t
 * @param array The array object to search in.
 * @param needle The ID to search for.
 * @param[out] ret_index If set, this will receive the index. Set to NULL if you're not interested in the index.
 * @return 1=ID is present in array, 0=ID not found.
 */
int              nc_array_search_id          (const nc_array_t* array, uint32_t needle, size_t* ret_index);


/**
 * @class nc_chatlist_t
 *
 * An object representing a single chatlist in memory.
 * Chatlist objects contain chat IDs
 * and, if possible, message IDs belonging to them.
 * The chatlist object is not updated;
 * if you want an update, you have to recreate the object.
 *
 * For a **typical chat overview**,
 * the idea is to get the list of all chats via nc_get_chatlist()
 * without any listflags (see below)
 * and to implement a "virtual list" or so
 * (the count of chats is known by nc_chatlist_get_cnt()).
 *
 * Only for the items that are in view
 * (the list may have several hundreds chats),
 * the UI should call nc_chatlist_get_summary() then.
 * nc_chatlist_get_summary() provides all elements needed for painting the item.
 *
 * On a click of such an item,
 * the UI should change to the chat view
 * and get all messages from this view via nc_get_chat_msgs().
 * Again, a "virtual list" is created
 * (the count of messages is known)
 * and for each messages that is scrolled into view, nc_get_msg() is called then.
 *
 * Why no listflags?
 * Without listflags, nc_get_chatlist() adds
 * the archive "link" automatically as needed.
 * The UI can just render these items differently then.
 */


/**
 * Free a chatlist object.
 *
 * @memberof nc_chatlist_t
 * @param chatlist The chatlist object to free, created e.g. by nc_get_chatlist(), nc_search_msgs().
 *     If NULL is given, nothing is done.
 */
void             nc_chatlist_unref           (nc_chatlist_t* chatlist);


/**
 * Find out the number of chats in a chatlist.
 *
 * @memberof nc_chatlist_t
 * @param chatlist The chatlist object as created e.g. by nc_get_chatlist().
 * @return Returns the number of items in a nc_chatlist_t object. 0 on errors or if the list is empty.
 */
size_t           nc_chatlist_get_cnt         (const nc_chatlist_t* chatlist);


/**
 * Get a single chat ID of a chatlist.
 *
 * To get the message object from the message ID, use nc_get_chat().
 *
 * @memberof nc_chatlist_t
 * @param chatlist The chatlist object as created e.g. by nc_get_chatlist().
 * @param index The index to get the chat ID for.
 * @return Returns the chat ID of the item at the given index. Index must be between
 *     0 and nc_chatlist_get_cnt()-1.
 */
uint32_t         nc_chatlist_get_chat_id     (const nc_chatlist_t* chatlist, size_t index);


/**
 * Get a single message ID of a chatlist.
 *
 * To get the message object from the message ID, use nc_get_msg().
 *
 * @memberof nc_chatlist_t
 * @param chatlist The chatlist object as created e.g. by nc_get_chatlist().
 * @param index The index to get the chat ID for.
 * @return Returns the message_id of the item at the given index. Index must be between
 *     0 and nc_chatlist_get_cnt()-1. If there is no message at the given index (e.g. the chat may be empty), 0 is returned.
 */
uint32_t         nc_chatlist_get_msg_id      (const nc_chatlist_t* chatlist, size_t index);


/**
 * Get a summary for a chatlist index.
 *
 * The summary is returned by a nc_lot_t object with the following fields:
 *
 * - nc_lot_t::text1: contains the username or the strings "Me", "Draft" and so on.
 *   The string may be colored by having a look at text1_meaning.
 *   If there is no such name or it should not be displayed, the element is NULL.
 *
 * - nc_lot_t::text1_meaning: one of NC_TEXT1_USERNAME, NC_TEXT1_SELF or NC_TEXT1_DRAFT.
 *   Typically used to show nc_lot_t::text1 with different colors. 0 if not applicable.
 *
 * - nc_lot_t::text2: contains an excerpt of the message text or strings as
 *   "No messages". May be NULL of there is no such text (e.g. for the archive link)
 *
 * - nc_lot_t::timestamp: the timestamp of the message. 0 if not applicable.
 *
 * - nc_lot_t::state: The state of the message as one of the @ref NC_STATE constants. 0 if not applicable.
 *
 * @memberof nc_chatlist_t
 * @param chatlist The chatlist to query as returned e.g. from nc_get_chatlist().
 * @param index The index to query in the chatlist.
 * @param chat To speed up things, pass an already available chat object here.
 *     If the chat object is not yet available, it is faster to pass NULL.
 * @return The summary as an nc_lot_t object. Must be freed using nc_lot_unref(). NULL is never returned.
 */
nc_lot_t*        nc_chatlist_get_summary     (const nc_chatlist_t* chatlist, size_t index, nc_chat_t* chat);


/**
 * Create a chatlist summary item when the chatlist object is already unref()'d.
 *
 * This function is similar to nc_chatlist_get_summary(), however,
 * it takes the chat ID and the message ID as returned by nc_chatlist_get_chat_id() and nc_chatlist_get_msg_id()
 * as arguments. The chatlist object itself is not needed directly.
 *
 * This maybe useful if you convert the complete object into a different representation
 * as done e.g. in the node-bindings.
 * If you have access to the chatlist object in some way, using this function is not recommended,
 * use nc_chatlist_get_summary() in this case instead.
 *
 * @memberof nc_context_t
 * @param context The context object.
 * @param chat_id The chat ID to get a summary for.
 * @param msg_id The message ID to get a summary for.
 * @return The summary as an nc_lot_t object, see nc_chatlist_get_summary() for details.
 *     Must be freed using nc_lot_unref(). NULL is never returned.
 */
nc_lot_t*        nc_chatlist_get_summary2    (nc_context_t* context, uint32_t chat_id, uint32_t msg_id);


/**
 * Helper function to get the associated context object.
 *
 * @memberof nc_chatlist_t
 * @param chatlist The chatlist object to empty.
 * @return The context object associated with the chatlist. NULL if none or on errors.
 */
nc_context_t*    nc_chatlist_get_context     (nc_chatlist_t* chatlist);


/**
 * Get info summary for a chat, in JSON format.
 *
 * The returned JSON string has the following key/values:
 *
 * id: chat id
 * name: chat/group name
 * color: color of this chat
 * last-message-from: who sent the last message
 * last-message-text: message (truncated)
 * last-message-state: @ref NC_STATE constant
 * last-message-date:
 * avatar-path: path-to-blobfile
 * is_verified: yes/no
 * @return a UTF8-encoded JSON string containing all requested info. Must be freed using nc_str_unref(). NULL is never returned.
 */
char*            nc_chat_get_info_json       (nc_context_t* context, size_t chat_id);

/**
 * @class nc_chat_t
 *
 * An object representing a single chat in memory.
 * Chat objects are created using e.g. nc_get_chat()
 * and are not updated on database changes;
 * if you want an update, you have to recreate the object.
 */


#define         NC_CHAT_ID_TRASH             3 // messages that should be deleted get this chat ID; the messages are deleted from the working thread later then. This is also needed as rfc724_mid should be preset as long as the message is not deleted on the server (otherwise it is downloaded again)
#define         NC_CHAT_ID_ARCHIVED_LINK     6 // only an indicator in a chatlist
#define         NC_CHAT_ID_ALLDONE_HINT      7 // only an indicator in a chatlist
#define         NC_CHAT_ID_LAST_SPECIAL      9 // larger chat IDs are "real" chats, their messages are "real" messages


/**
 * Free a chat object.
 *
 * @memberof nc_chat_t
 * @param chat Chat object are returned e.g. by nc_get_chat().
 *     If NULL is given, nothing is done.
 */
void            nc_chat_unref                (nc_chat_t* chat);


/**
 * Get the chat ID. The chat ID is the ID under which the chat is filed in the database.
 *
 * Special IDs:
 * - NC_CHAT_ID_ARCHIVED_LINK    (6) - A link at the end of the chatlist, if present the UI should show the button "Archived chats"-
 *
 * "Normal" chat IDs are larger than these special IDs (larger than NC_CHAT_ID_LAST_SPECIAL).
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return The chat ID. 0 on errors.
 */
uint32_t        nc_chat_get_id               (const nc_chat_t* chat);


/**
 * Get chat type as one of the @ref NC_CHAT_TYPE constants.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return The chat type.
 */
int             nc_chat_get_type             (const nc_chat_t* chat);


/**
 * Returns the address where messages are sent to if the chat is a mailing list.
 * If you just want to know if a mailing list can be written to,
 * use nc_chat_can_send() instead.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return The mailing list address. Must be released using nc_str_unref() after usage.
 *     If there is no such address, an empty string is returned, NULL is never returned.
 */
char*           nc_chat_get_mailinglist_addr (const nc_chat_t* chat);


/**
 * Get name of a chat. For one-to-one chats, this is the name of the contact.
 * For group chats, this is the name given e.g. to nc_create_group_chat() or
 * received by a group-creation message.
 *
 * To change the name, use nc_set_chat_name()
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return The chat name as a string. Must be released using nc_str_unref() after usage. Never NULL.
 */
char*           nc_chat_get_name             (const nc_chat_t* chat);


/**
 * Get the chat's profile image.
 * For groups, this is the image set by any group member
 * using nc_set_chat_profile_image().
 * For normal chats, this is the image set by each remote user on their own
 * using nc_set_config(context, "selfavatar", image).
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return Path and file of the profile image, if any.
 *     NULL otherwise.
 *     Must be released using nc_str_unref() after usage.
 */
char*           nc_chat_get_profile_image    (const nc_chat_t* chat);


/**
 * Get a color for the chat.
 * For 1:1 chats, the color is calculated from the contact's e-mail address.
 * Otherwise, the chat name is used.
 * The color can be used for an fallback avatar with white initials
 * as well as for headlines in bubbles of group chats.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return The color as 0x00rrggbb with rr=red, gg=green, bb=blue
 *     each in the range 0-255.
 */
uint32_t        nc_chat_get_color            (const nc_chat_t* chat);


/**
 * Get visibility of chat.
 * See @ref NC_CHAT_VISIBILITY for detailed information about the visibilities.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return One of @ref NC_CHAT_VISIBILITY
 */
int             nc_chat_get_visibility       (const nc_chat_t* chat);


/**
 * Check if a chat is a contact request chat.
 *
 * UI should display such chats with a [New] badge in the chatlist.
 *
 * When such chat is opened, user should be presented with a set of
 * options instead of the message composition area, for example:
 * - Accept chat (nc_accept_chat())
 * - Block chat (nc_block_chat())
 * - Delete chat (nc_delete_chat())
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=chat is a contact request chat,
 *     0=chat is not a contact request chat.
 */
int             nc_chat_is_contact_request   (const nc_chat_t* chat);


/**
 * Check if a group chat is still unpromoted.
 *
 * After the creation with nc_create_group_chat() the chat is usually unpromoted
 * until the first call to nc_send_text_msg() or another sending function.
 *
 * With unpromoted chats, members can be added
 * and settings can be modified without the need of special status messages being sent.
 *
 * While the core takes care of the unpromoted state on its own,
 * checking the state from the UI side may be useful to decide whether a hint as
 * "Send the first message to allow others to reply within the group"
 * should be shown to the user or not.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=chat is still unpromoted, no message was ever send to the chat,
 *     0=chat is not unpromoted, messages were send and/or received
 *     or the chat is not group chat.
 */
int             nc_chat_is_unpromoted        (const nc_chat_t* chat);


/**
 * Check if a chat is a self talk. Self talks are normal chats with
 * the only contact NC_CONTACT_ID_SELF.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=chat is self talk, 0=chat is no self talk.
 */
int             nc_chat_is_self_talk         (const nc_chat_t* chat);


/**
 * Check if a chat is a device-talk.
 * Device-talks contain update information
 * and some hints that are added during the program runs, multi-device etc.
 *
 * From the UI view, device-talks are not very special,
 * the user can delete and forward messages, archive the chat, set notifications etc.
 *
 * Messages can be added to the device-talk using nc_add_device_msg()
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=chat is device talk, 0=chat is no device talk.
 */
int             nc_chat_is_device_talk       (const nc_chat_t* chat);


/**
 * Check if messages can be sent to a given chat.
 * This is not true e.g. for contact requests or for the device-talk, cmp. nc_chat_is_device_talk().
 *
 * Calling nc_send_msg() for these chats will fail
 * and the UI may decide to hide input controls therefore.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=chat is writable, 0=chat is not writable.
 */
int             nc_chat_can_send              (const nc_chat_t* chat);


/**
 * Deprecated, always returns 0.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return Always 0.
 * @deprecated 2025-09-09
 */
int             nc_chat_is_protected         (const nc_chat_t* chat);


/**
 * Check if the chat is encrypted.
 *
 * 1:1 chats with key-contacts and group chats with key-contacts
 * are encrypted.
 * 1:1 chats with emails contacts and ad-hoc groups
 * created for email threads are not encrypted.
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=chat is encrypted, 0=chat is not encrypted.
 */
int             nc_chat_is_encrypted         (const nc_chat_t *chat);


/**
 * Check if locations are sent to the chat
 * at the time the object was created using nc_get_chat().
 * To check if locations are sent to _any_ chat,
 * use nc_is_sending_locations_to_chat().
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=locations are sent to chat, 0=no locations are sent to chat.
 */
int             nc_chat_is_sending_locations (const nc_chat_t* chat);


/**
 * Check whether the chat is currently muted (can be changed by nc_set_chat_mute_duration()).
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 1=muted, 0=not muted
 */
int             nc_chat_is_muted (const nc_chat_t* chat);


/**
 * Get the exact state of the mute of a chat
 *
 * @memberof nc_chat_t
 * @param chat The chat object.
 * @return 0=not muted, -1=forever muted, (x>0)=remaining seconds until the mute is lifted.
 */
int64_t          nc_chat_get_remaining_mute_duration (const nc_chat_t* chat);


/**
 * @class nc_msg_t
 *
 * An object representing a single message in memory.
 * The message object is not updated.
 * If you want an update, you have to recreate the object.
 */


#define         NC_MSG_ID_MARKER1            1 // this is used by iOS to mark things in the message list
#define         NC_MSG_ID_DAYMARKER          9
#define         NC_MSG_ID_LAST_SPECIAL       9


/**
 * Create new message object. Message objects are needed e.g. for sending messages using
 * nc_send_msg(). Moreover, they are returned e.g. from nc_get_msg(),
 * set up with the current state of a message. The message object is not updated;
 * to achieve this, you have to recreate it.
 *
 * @memberof nc_msg_t
 * @param context The context that should be stored in the message object.
 * @param viewtype The type to the message object to create,
 *     one of the @ref NC_MSG constants.
 * @return The created message object.
 */
nc_msg_t*       nc_msg_new                    (nc_context_t* context, int viewtype);


/**
 * Free a message object. Message objects are created e.g. by nc_get_msg().
 *
 * @memberof nc_msg_t
 * @param msg The message object to free.
 *     If NULL is given, nothing is done.
 */
void            nc_msg_unref                  (nc_msg_t* msg);


/**
 * Get the ID of the message.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The ID of the message.
 *     0 if the given message object is invalid.
 */
uint32_t        nc_msg_get_id                 (const nc_msg_t* msg);


/**
 * Get the ID of the contact who wrote the message.
 *
 * If the ID is equal to NC_CONTACT_ID_SELF (1), the message is an outgoing
 * message that is typically shown on the right side of the chat view.
 *
 * Otherwise, the message is an incoming message; to get details about the sender,
 * pass the returned ID to nc_get_contact().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The ID of the contact who wrote the message, NC_CONTACT_ID_SELF (1)
 *     if this is an outgoing message, 0 on errors.
 */
uint32_t        nc_msg_get_from_id            (const nc_msg_t* msg);


/**
 * Get the ID of the chat the message belongs to.
 * To get details about the chat, pass the returned ID to nc_get_chat().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The ID of the chat the message belongs to, 0 on errors.
 */
uint32_t        nc_msg_get_chat_id            (const nc_msg_t* msg);


/**
 * Get the type of the message.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return One of the @ref NC_MSG constants.
 *     0 if the given message object is invalid.
 */
int             nc_msg_get_viewtype           (const nc_msg_t* msg);


/**
 * Get the state of a message.
 *
 * Incoming message states:
 * - @ref NC_STATE_IN_FRESH - Incoming _fresh_ message.
 *   Fresh messages are neither noticed nor seen and are typically shown in notifications.
 *   Use nc_get_fresh_msgs() to get all fresh messages.
 * - @ref NC_STATE_IN_NOTICED - Incoming _noticed_ message.
 *   E.g. chat opened but message not yet read.
 *   Noticed messages are not counted as unread but were not marked as read nor resulted in MDNs.
 *   Use nc_marknoticed_chat() to mark messages as being noticed.
 * - @ref NC_STATE_IN_SEEN - Incoming message, really _seen_ by the user.
 *   Marked as read on IMAP and MDN may be sent. Use nc_markseen_msgs() to mark messages as being seen.
 *
 * Outgoing message states:
 * - @ref NC_STATE_OUT_PREPARING - For files which need time to be prepared before they can be sent,
 *   the message enters this state before @ref NC_STATE_OUT_PENDING. Deprecated.
 * - @ref NC_STATE_OUT_DRAFT - Message saved as draft using nc_set_draft()
 * - @ref NC_STATE_OUT_PENDING - The user has pressed the "send" button but the
 *   message is not yet sent and is pending in some way. Maybe we're offline (no checkmark).
 * - @ref NC_STATE_OUT_FAILED - _Unrecoverable_ error (_recoverable_ errors result in pending messages),
 *   you will receive the event #NC_EVENT_MSG_FAILED.
 * - @ref NC_STATE_OUT_DELIVERED - Outgoing message successfully delivered to server (one checkmark).
 *   Note, that already delivered messages may get into the state @ref NC_STATE_OUT_FAILED if we get such a hint from the server.
 *   If a sent message changes to this state, you will receive the event #NC_EVENT_MSG_DELIVERED.
 * - @ref NC_STATE_OUT_MDN_RCVD - Outgoing message read by the recipient
 *   (two checkmarks; this requires goodwill on the receiver's side)
 *   If a sent message changes to this state, you will receive the event #NC_EVENT_MSG_READ.
 *   Also messages already read by some recipients
 *   may get into the state @ref NC_STATE_OUT_FAILED at a later point,
 *   e.g. when in a group, delivery fails for some recipients.
 *
 * If you just want to check if a message is sent or not, please use nc_msg_is_sent() which regards all states accordingly.
 *
 * The state of just created message objects is @ref NC_STATE_UNDEFINED.
 * The state is always set by the core library, users of the library cannot set the state directly, but it is changed implicitly e.g.
 * when calling nc_marknoticed_chat() or nc_markseen_msgs().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The state of the message.
 */
int             nc_msg_get_state              (const nc_msg_t* msg);


/**
 * Get the message sending time.
 * The sending time is returned as a unix timestamp in seconds.
 *
 * Note that the message lists returned e.g. by nc_get_chat_msgs()
 * are not sorted by the _sending_ time but by the _receiving_ time.
 * This ensures newly received messages always pop up at the end of the list,
 * however, for delayed messages, the correct sending time will be displayed.
 *
 * To display detailed information about the times to the user,
 * the UI can use nc_get_msg_info().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The time of the message.
 */
int64_t          nc_msg_get_timestamp          (const nc_msg_t* msg);


/**
 * Get the message receive time.
 * The receive time is returned as a unix timestamp in seconds.
 *
 * To get the sending time, use nc_msg_get_timestamp().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The receiving time of the message.
 *     For outgoing messages, 0 is returned.
 */
int64_t          nc_msg_get_received_timestamp (const nc_msg_t* msg);


/**
 * Get the message time used for sorting.
 * This function returns the timestamp that is used for sorting the message
 * into lists as returned e.g. by nc_get_chat_msgs().
 * This may be the received time, the sending time or another time.
 *
 * To get the receiving time, use nc_msg_get_received_timestamp().
 * To get the sending time, use nc_msg_get_timestamp().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The time used for ordering.
 */
int64_t          nc_msg_get_sort_timestamp     (const nc_msg_t* msg);


/**
 * Get the text of the message.
 * If there is no text associated with the message, an empty string is returned.
 * NULL is never returned.
 *
 * The returned text is plain text, HTML is stripped.
 * The returned text is truncated to a max. length of currently about 30000 characters,
 * it does not make sense to show more text in the message list and typical controls
 * will have problems with showing much more text.
 * This max. length is to avoid passing _lots_ of data to the frontend which may
 * result e.g. from decoding errors (assume some bytes missing in a mime structure, forcing
 * an attachment to be plain text).
 *
 * To get information about the message and more/raw text, use nc_get_msg_info().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The message text. The result must be released using nc_str_unref(). Never returns NULL.
 */
char*           nc_msg_get_text               (const nc_msg_t* msg);


/**
 * Get the subject of the e-mail.
 * If there is no subject associated with the message, an empty string is returned.
 * NULL is never returned.
 *
 * You usually don't need this; if the core thinks that the subject might contain important
 * information, it automatically prepends it to the message text.
 *
 * This function was introduced so that you can use the subject as the title for the 
 * full-message-view (see nc_get_msg_html()).
 *
 * For outgoing messages, the subject is not stored and an empty string is returned.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The subject. The result must be released using nc_str_unref(). Never returns NULL.
 */
char*           nc_msg_get_subject            (const nc_msg_t* msg);


/**
 * Find out full path of the file associated with a message.
 *
 * Typically files are associated with images, videos, audios, documents.
 * Plain text messages do not have a file.
 * File name may be mangled. To obtain the original attachment filename use nc_msg_get_filename().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The full path (with file name and extension) of the file associated with the message.
 *     If there is no file associated with the message, an empty string is returned.
 *     NULL is never returned and the returned value must be released using nc_str_unref().
 */
char*           nc_msg_get_file               (const nc_msg_t* msg);


/**
 * Save file copy at the user-provided path.
 *
 * Fails if file already exists at the provided path.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param path Destination file path with filename and extension.
 * @return 0 on failure, 1 on success.
 */
int             nc_msg_save_file              (const nc_msg_t* msg, const char* path);


/**
 * Get an original attachment filename, with extension but without the path. To get the full path,
 * use nc_msg_get_file().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The attachment filename. If there is no file associated with the message, an empty string
 *     is returned. The returned value must be released using nc_str_unref().
 */
char*           nc_msg_get_filename           (const nc_msg_t* msg);


/**
 * Get the MIME type of a file. If there is no file, an empty string is returned.
 * If there is no associated MIME type with the file, the function guesses on; if
 * in doubt, `application/octet-stream` is returned. NULL is never returned.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return A string containing the MIME type.
 *     Must be released using nc_str_unref() after usage. NULL is never returned.
 */
char*           nc_msg_get_filemime           (const nc_msg_t* msg);


/**
 * Return a file from inside a webxnc message.
 *
 * @memberof nc_msg_t
 * @param msg The webxnc instance.
 * @param filename The name inside the archive.
 *     Can be given as an absolute path (`/file.png`)
 *     or as a relative path (`file.png`, no leading slash).
 * @param ret_bytes A pointer to a size_t. The size of the blob will be written here.
 * @return The blob must be released using nc_str_unref() after usage.
 *     NULL if there is no such file in the archive or on errors.
 */
char*             nc_msg_get_webxnc_blob      (const nc_msg_t* msg, const char* filename, size_t* ret_bytes);


/**
 * Get info from a webxnc message, in JSON format.
 * The returned JSON string has the following key/values:
 *
 * - name: The name of the app.
 *   Defaults to the filename if not set in the manifest.
 * - icon: App icon file name.
 *   Defaults to an standard icon if nothing is set in the manifest.
 *   To get the file, use nc_msg_get_webxnc_blob().
 *   App icons should should be square,
 *   the implementations will add round corners etc. as needed.
 * - document: if the Webxnc represents a document, this is the name of the document,
 *   otherwise, this is an empty string.
 * - summary: short string describing the state of the app,
 *   sth. as "2 votes", "Highscore: 123",
 *   can be changed by the apps and defaults to an empty string.
 * - source_code_url:
 *   URL where the source code of the Webxnc and other information can be found;
 *   defaults to an empty string.
 *   Implementations may offer an menu or a button to open this URL.
 * - internet_access:
 *   true if the Webxnc should get internet access;
 *   this is the case i.e. for experimental maps integration.
 * - self_addr: address to be used for `window.webxnc.selfAddr` in JS land.
 * - send_update_interval: Milliseconds to wait before calling `sendUpdate()` again since the last call.
 *   Should be exposed to `webxnc.sendUpdateInterval` in JS land.
 * - send_update_max_size: Maximum number of bytes accepted for a serialized update object.
 *   Should be exposed to `webxnc.sendUpdateMaxSize` in JS land.
 *
 * @memberof nc_msg_t
 * @param msg The webxnc instance.
 * @return A UTF8 encoded JSON string containing all requested info.
 *     Must be freed using nc_str_unref().
 *     NULL is never returned.
 */
char*             nc_msg_get_webxnc_info      (const nc_msg_t* msg);


/**
 * Get the size of the file. Returns the size of the file associated with a
 * message, if applicable.
 * If message is a pre-message, then this returns the size of the file to be downloaded.
 *
 * Typically, this is used to show the size of document files, e.g. a PDF.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The file size in bytes, 0 if not applicable or on errors.
 */
uint64_t        nc_msg_get_filebytes          (const nc_msg_t* msg);


/**
 * Get the width of an image or a video. The width is returned in pixels.
 * If the width is unknown or if the associated file is no image or video file,
 * 0 is returned.
 *
 * Often the aspect ratio is the more interesting thing. You can calculate
 * this using nc_msg_get_width() / nc_msg_get_height().
 *
 * See also nc_msg_get_duration().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The width in pixels, if applicable. 0 otherwise or if unknown.
 */
int             nc_msg_get_width              (const nc_msg_t* msg);


/**
 * Get the height of an image or a video. The height is returned in pixels.
 * If the height is unknown or if the associated file is no image or video file,
 * 0 is returned.
 *
 * Often the aspect ratio is the more interesting thing. You can calculate
 * this using nc_msg_get_width() / nc_msg_get_height().
 *
 * See also nc_msg_get_duration().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The height in pixels, if applicable. 0 otherwise or if unknown.
 */
int             nc_msg_get_height             (const nc_msg_t* msg);


/**
 * Get the duration of audio or video. The duration is returned in milliseconds (ms).
 * If the duration is unknown or if the associated file is no audio or video file,
 * 0 is returned.
 *
 * See also nc_msg_get_width() and nc_msg_get_height().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The duration in milliseconds, if applicable. 0 otherwise or if unknown.
 */
int             nc_msg_get_duration           (const nc_msg_t* msg);


/**
 * Check if message was correctly encrypted and signed.
 *
 * Historically, UIs showed a small padlock on the message then.
 * Today, the UIs should instead
 * show a small email-icon on the message if the message is not encrypted or signed,
 * and nothing otherwise.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=message correctly encrypted and signed, no need to show anything; 0=show email-icon beside the message.
 */
int             nc_msg_get_showpadlock        (const nc_msg_t* msg);

/**
 * Check if an incoming message is a bot message, i.e. automatically submitted.
 *
 * Return value for outgoing messages is unspecified.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=message is submitted automatically, 0=message is not automatically submitted.
 */
int             nc_msg_is_bot                 (const nc_msg_t* msg); 

/**
 * Get the ephemeral timer duration for a message.
 * This is the value of nc_get_chat_ephemeral_timer() in the moment the message was sent.
 *
 * To check if the timer is started and calculate remaining time,
 * use nc_msg_get_ephemeral_timestamp().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The duration in seconds, or 0 if no timer is set.
 */
uint32_t        nc_msg_get_ephemeral_timer    (const nc_msg_t* msg);

/**
 * Get the timestamp of the ephemeral message removal.
 *
 * If returned value is non-zero, you can calculate the * fraction of
 * time remaining by divinding the difference between the current timestamp
 * and this timestamp by nc_msg_get_ephemeral_timer().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The time of the message removal, 0 if the timer is not yet started.
 *     (The timer starts on sending messages or when nc_markseen_msgs() is called.)
 */
int64_t          nc_msg_get_ephemeral_timestamp (const nc_msg_t* msg);


/**
 * Get a summary for a message.
 *
 * The summary is returned by a nc_lot_t object with the following fields:
 *
 * - nc_lot_t::text1: contains the username or the string "Me".
 *   The string may be colored by having a look at text1_meaning.
 *   If the name should not be displayed, the element is NULL.
 * - nc_lot_t::text1_meaning: one of NC_TEXT1_USERNAME or NC_TEXT1_SELF.
 *   Typically used to show nc_lot_t::text1 with different colors. 0 if not applicable.
 * - nc_lot_t::text2: contains an excerpt of the message text.
 * - nc_lot_t::timestamp: the timestamp of the message.
 * - nc_lot_t::state: The state of the message as one of the @ref NC_STATE constants.
 *
 * Typically used to display a search result. See also nc_chatlist_get_summary() to display a list of chats.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param chat The chat object. To speed up things, pass an already available chat object here.
 *     If the chat object is not yet available, it is faster to pass NULL.
 * @return The summary as an nc_lot_t object. Must be freed using nc_lot_unref(). NULL is never returned.
 */
nc_lot_t*       nc_msg_get_summary            (const nc_msg_t* msg, const nc_chat_t* chat);


/**
 * Get a message summary as a single line of text. Typically used for
 * notifications.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param approx_characters A rough length of the expected string.
 * @return A summary for the given messages.
 *     The returned string must be released using nc_str_unref().
 *     Returns an empty string on errors, never returns NULL.
 */
char*           nc_msg_get_summarytext        (const nc_msg_t* msg, int approx_characters);


/**
 * Get the name that should be shown over the message (in a group chat) instead of the contact
 * display name, or NULL.
 *
 * If this returns non-NULL, put a `~` before the override-sender-name and show the
 * override-sender-name and the sender's avatar even in 1:1 chats.
 *
 * In mailing lists, sender display name and sender address do not always belong together.
 * In this case, this function gives you the name that should actually be shown over the message.
 *
 * Also, sometimes, we need to indicate a different sender in 1:1 chats:
 * Suppose that our user writes an e-mail to support@nexus.chat, which forwards to 
 * Bob <bob@nexus.chat>, and Bob replies.
 * 
 * Then, Bob's reply is shown in our 1:1 chat with support@nexus.chat and the override-sender-name is
 * set to `Bob`. The UI should show the sender name as `~Bob` and show the avatar, just
 * as in group messages. If the user then taps on the avatar, they can see that this message
 * comes from bob@nexus.chat.
 * 
 * You should show a `~` before the override-sender-name in chats, so that the user can
 * see that this isn't the sender's actual name.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The name to show over this message or NULL.
 *     If this returns NULL, call `nc_contact_get_display_name()`.
 *     The returned string must be released using nc_str_unref().
 */
char*           nc_msg_get_override_sender_name(const nc_msg_t* msg);



/**
 * Check if a message has a deviating timestamp.
 * A message has a deviating timestamp
 * when it is sent on another day as received/sorted by.
 *
 * When the UI displays normally only the time beside the message and the full day as headlines,
 * the UI should display the full date directly beside the message if the timestamp is deviating.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=Timestamp is deviating, the UI should display the full date beside the message,
 *     0=Timestamp is not deviating and belongs to the same date as the date headers,
 *     displaying the time only is sufficient in this case.
 */
int             nc_msg_has_deviating_timestamp(const nc_msg_t* msg);


/**
 * Check if a message has a POI location bound to it.
 * These locations are also returned by nc_get_locations()
 * The UI may decide to display a special icon beside such messages.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=Message has location bound to it, 0=No location bound to message.
 */
int             nc_msg_has_location           (const nc_msg_t* msg);


/**
 * Check if a message was sent successfully.
 *
 * Currently, "sent" messages are messages that are in the state "delivered" or "mdn received",
 * see nc_msg_get_state().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=message sent successfully, 0=message not yet sent or message is an incoming message.
 */
int             nc_msg_is_sent                (const nc_msg_t* msg);


/**
 * Check if the message is a forwarded message.
 *
 * Forwarded messages may not be created by the contact given as "from".
 *
 * Typically, the UI shows a little text for a symbol above forwarded messages.
 *
 * For privacy reasons, we do not provide the name or the e-mail address of the
 * original author (in a typical GUI, you select the messages text and click on
 * "forwarded"; you won't expect other data to be send to the new recipient,
 * esp. as the new recipient may not be in any relationship to the original author)
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=message is a forwarded message, 0=message not forwarded.
 */
int             nc_msg_is_forwarded           (const nc_msg_t* msg);


/**
 * Check if the message was edited.
 *
 * Edited messages should be marked by the UI as such,
 * e.g. by the text "Edited" beside the time.
 * To edit messages, use nc_send_edit_request().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=message is edited, 0=message not edited.
 */
 int             nc_msg_is_edited             (const nc_msg_t* msg);


/**
 * Check if the message is an informational message, created by the
 * device or by another users. Such messages are not "typed" by the user but
 * created due to other actions,
 * e.g. nc_set_chat_name(), nc_set_chat_profile_image(),
 * or nc_add_contact_to_chat().
 *
 * These messages are typically shown in the center of the chat view,
 * nc_msg_get_text() returns a descriptive text about what is going on.
 *
 * For informational messages created by Webxnc apps,
 * nc_msg_get_parent() usually returns the Webxnc instance;
 * UIs can use that to scroll to the Webxnc app when the info is tapped.
 *
 * There is no need to perform any action when seeing such a message - this is already done by the core.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 1=message is a system command, 0=normal message.
 */
int             nc_msg_is_info                (const nc_msg_t* msg);


/**
 * Get the type of an informational message.
 * If nc_msg_is_info() returns 1, this function returns the type of the informational message.
 * UIs can display e.g. an icon based upon the type.
 *
 * Currently, the following types are defined:
 * - NC_INFO_GROUP_NAME_CHANGED (2) - "Group name changd from OLD to BY by CONTACT"
 * - NC_INFO_GROUP_IMAGE_CHANGED (3) - "Group image changd by CONTACT"
 * - NC_INFO_MEMBER_ADDED_TO_GROUP (4) - "Member CONTACT added by OTHER_CONTACT"
 * - NC_INFO_MEMBER_REMOVED_FROM_GROUP (5) - "Member CONTACT removed by OTHER_CONTACT"
 * - NC_INFO_EPHEMERAL_TIMER_CHANGED (10) - "Disappearing messages CHANGED_TO by CONTACT"
 * - NC_INFO_PROTECTION_ENABLED (11) - Info-message for "Chat is protected"
 * - NC_INFO_INVALID_UNENCRYPTED_MAIL (13) - Info-message for "Provider requires end-to-end encryption which is not setup yet",
 *   the UI should change the corresponding string using #NC_STR_INVALID_UNENCRYPTED_MAIL
 *   and also offer a way to fix the encryption, eg. by a button offering a QR scan
 * - NC_INFO_WEBXNC_INFO_MESSAGE (32) - Info-message created by webxnc app sending `update.info`
 * - NC_INFO_CHAT_E2EE (50) - Info-message for "Chat is end-to-end-encrypted"
 * - NC_INFO_GROUP_DESCRIPTION_CHANGED (70) - Info-message "Description changed", UI should open the profile with the description
 *
 * For the messages that refer to a CONTACT,
 * nc_msg_get_info_contact_id() returns the contact ID.
 * The UI should open the contact's profile when tapping the info message.
 *
 * Even when you display an icon,
 * you should still display the text of the informational message using nc_msg_get_text()
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return One of the NC_INFO* constants.
 *     0 or other values indicate unspecified types
 *     or that the message is not an info-message.
 */
int             nc_msg_get_info_type          (const nc_msg_t* msg);


/**
 * Return the contact ID of the profile to open when tapping the info message.
 *
 * - For NC_INFO_MEMBER_ADDED_TO_GROUP and NC_INFO_MEMBER_REMOVED_FROM_GROUP,
 *   this is the contact being added/removed.
 *   The contact that did the adding/removal is usually only a tap away
 *   (as introducer and/or atop of the memberlist),
 *   and usually more known anyways.
 * - For NC_INFO_GROUP_NAME_CHANGED, NC_INFO_GROUP_IMAGE_CHANGED and NC_INFO_EPHEMERAL_TIMER_CHANGED
 *   this is the contact who did the change.
 *
 * No need to check additionally for nc_msg_get_info_type(),
 * unless you e.g. want to show the info message in another style.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return If the info message refers to a contact,
 *     this contact ID or NC_CONTACT_ID_SELF is returned.
 *     Otherwise 0.
 */
uint32_t        nc_msg_get_info_contact_id    (const nc_msg_t* msg);


// NC_INFO* uses the same values as SystemMessage in rust-land
#define         NC_INFO_UNKNOWN                    0
#define         NC_INFO_GROUP_NAME_CHANGED         2
#define         NC_INFO_GROUP_IMAGE_CHANGED        3
#define         NC_INFO_MEMBER_ADDED_TO_GROUP      4
#define         NC_INFO_MEMBER_REMOVED_FROM_GROUP  5

// Deprecated as of 2026-03-16, not used for new messages.
#define         NC_INFO_AUTOCRYPT_SETUP_MESSAGE    6

#define         NC_INFO_SECURE_JOIN_MESSAGE        7
#define         NC_INFO_LOCATIONSTREAMING_ENABLED  8
#define         NC_INFO_LOCATION_ONLY              9
#define         NC_INFO_EPHEMERAL_TIMER_CHANGED   10
#define         NC_INFO_PROTECTION_ENABLED        11
#define         NC_INFO_INVALID_UNENCRYPTED_MAIL  13
#define         NC_INFO_WEBXNC_INFO_MESSAGE       32
#define         NC_INFO_CHAT_E2EE                 50
#define         NC_INFO_GROUP_DESCRIPTION_CHANGED 70


/**
 * Get link attached to an webxnc info message.
 * The info message needs to be of type NC_INFO_WEBXNC_INFO_MESSAGE.
 *
 * Typically, this is used to set `document.location.href` in JS land.
 *
 * Webxnc apps can define the link by setting `update.href` when sending and update,
 * see nc_send_webxnc_status_update().
 *
 * @memberof nc_msg_t
 * @param msg The info message object.
 *     Not: the webxnc instance.
 * @return The link to be set to `document.location.href` in JS land.
 *     Returns NULL if there is no link attached to the info message and on errors.
 */
char*           nc_msg_get_webxnc_href        (const nc_msg_t* msg);


/**
 * Gets the error status of the message.
 * If there is no error associated with the message, NULL is returned.
 *
 * A message can have an associated error status if something went wrong when sending or
 * receiving message itself. The error status is free-form text and should not be further parsed,
 * rather it's presence is meant to indicate *something* went wrong with the message and the
 * text of the error is detailed information on what.
 * 
 * Some common reasons error can be associated with messages are:
 * * Lack of valid signature on an e2ee message, usually for received messages.
 * * Failure to decrypt an e2ee message, usually for received messages.
 * * When a message could not be delivered to one or more recipients the non-delivery
 *   notification text can be stored in the error status.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return An error or NULL. The result must be released using nc_str_unref().
 */
char*           nc_msg_get_error               (const nc_msg_t* msg);


/**
 * Checks if the message has a full HTML version.
 *
 * Messages have a full HTML version
 * if the original message _may_ contain important parts
 * that are removed by some heuristics
 * or if the message is just too long or too complex
 * to get displayed properly by just using plain text.
 * If so, the UI should offer a button as
 * "Show full message" that shows the uncut message using nc_get_msg_html().
 *
 * Even if a "Show full message" button is recommended,
 * the UI should display the text in the bubble
 * using the normal nc_msg_get_text() function -
 * which will still be fine in many cases.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return 0=Message as displayed using nc_msg_get_text() is just fine;
 *     1=Message has a full HTML version,
 *     should be displayed using nc_msg_get_text()
 *     and a button to show the full version should be offered
 */
int nc_msg_has_html (nc_msg_t* msg);


/**
  * Check if the message is completely downloaded
  * or if some further action is needed.
  *
  * Messages may be not fully downloaded
  * if they are larger than the limit set by the nc_set_config()-option `download_limit`.
  *
  * The function returns one of:
  * - @ref NC_DOWNLOAD_DONE           - The message does not need any further download action
  *                                     and should be rendered as usual.
  * - @ref NC_DOWNLOAD_AVAILABLE      - There is additional content to download.
  *                                     In addition to the usual message rendering,
  *                                     the UI shall show a download button that calls nc_download_full_msg()
  * - @ref NC_DOWNLOAD_IN_PROGRESS    - Download was started with nc_download_full_msg() and is still in progress.
  *                                     If the download fails or succeeds,
  *                                     the event @ref NC_EVENT_MSGS_CHANGED is emitted.
  *
  * - @ref NC_DOWNLOAD_UNDECIPHERABLE - The message does not need any further download action.
  *                                     It was fully downloaded, but we failed to decrypt it.
  * - @ref NC_DOWNLOAD_FAILURE        - Download error, the user may start over calling nc_download_full_msg() again.
  *
  * @memberof nc_msg_t
  * @param msg The message object.
  * @return One of the @ref NC_DOWNLOAD values.
  */
int nc_msg_get_download_state (const nc_msg_t* msg);


/**
 * Set the text of a message object.
 * This does not alter any information in the database; this may be done by nc_send_msg() later.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param text The message text.
 */
void            nc_msg_set_text               (nc_msg_t* msg, const char* text);


/**
 * Set the HTML part of a message object.
 * As for all other nc_msg_t setters,
 * this is only useful if the message is sent using nc_send_msg() later.
 *
 * Please note, that Nexus Chat clients show the plain text set with
 * nc_msg_set_text() at the first place;
 * the HTML part is not shown instead of this text.
 * However, for messages with HTML parts,
 * on the receiver's device, nc_msg_has_html() will return 1
 * and a button "Show full message" is typically shown.
 *
 * So adding a HTML part might be useful e.g. for bots,
 * that want to add rich content to a message, e.g. a website;
 * this HTML part is similar to an attachment then.
 *
 * **nc_msg_set_html() is currently not meant for sending a message,
 * a "normal user" has typed in!**
 * Use nc_msg_set_text() for that purpose.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param html HTML to send.
 */
void            nc_msg_set_html               (nc_msg_t* msg, const char* html);


/**
 * Sets the email's subject. If it's empty, a default subject
 * will be used (e.g. `Message from Alice` or `Re: <last subject>`).
 * This does not alter any information in the database.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param subject The new subject.
 */
void            nc_msg_set_subject            (nc_msg_t* msg, const char* subject);


/**
 * Set different sender name for a message.
 * This overrides the name set by the nc_set_config()-option `displayname`.
 *
 * Usually, this function is not needed
 * when implementing pure messaging functions.
 * However, it might be useful for bots e.g. building bridges to other networks.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param name The name to send along with the message.
 */
void            nc_msg_set_override_sender_name(nc_msg_t* msg, const char* name);


/**
 * Sets the file associated with a message.
 *
 * If `name` is non-null, it is used as the file name
 * and the actual current name of the file is ignored.
 *
 * If the source file is already in the blobdir, it will be renamed,
 * otherwise it will be copied to the blobdir first.
 *
 * In order to deduplicate files that contain the same data,
 * the file will be named `<hash>.<extension>`, e.g. `ce940175885d7b78f7b7e9f1396611f.jpg`.
 *
 * NOTE:
 * - This function will rename the file. To get the new file path, call `get_file()`.
 * - The file must not be modified after this function was called.
 *
 * @memberof nc_msg_t
 * @param msg The message object. Must not be NULL.
 * @param file The path of the file to attach. Must not be NULL.
 * @param name The original filename of the attachment. If NULL, the current name of `file` will be used instead.
 * @param filemime The MIME type of the file. NULL if you don't know or don't care.
 */
void            nc_msg_set_file_and_deduplicate(nc_msg_t* msg, const char* file, const char* name, const char* filemime);


/**
 * Set the dimensions associated with message object.
 * Typically this is the width and the height of an image or video associated using nc_msg_set_file_and_deduplicate().
 * This does not alter any information in the database; this may be done by nc_send_msg() later.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param width The width in pixels, if known. 0 if you don't know or don't care.
 * @param height The height in pixels, if known. 0 if you don't know or don't care.
 */
void            nc_msg_set_dimension          (nc_msg_t* msg, int width, int height);


/**
 * Set the duration associated with message object.
 * Typically this is the duration of an audio or video associated using nc_msg_set_file_and_deduplicate().
 * This does not alter any information in the database; this may be done by nc_send_msg() later.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param duration The duration in milliseconds. 0 if you don't know or don't care.
 */
void            nc_msg_set_duration           (nc_msg_t* msg, int duration);


/**
 * Set any location that should be bound to the message object.
 * The function is useful to add a marker to the map
 * at a position different from the self-location.
 * You should not call this function
 * if you want to bind the current self-location to a message;
 * this is done by nc_set_location() and nc_send_locations_to_chat().
 *
 * Typically results in the event #NC_EVENT_LOCATION_CHANGED with
 * contact ID set to NC_CONTACT_ID_SELF.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param latitude A north-south position of the location.
 * @param longitude An east-west position of the location.
 */
void            nc_msg_set_location           (nc_msg_t* msg, double latitude, double longitude);


/**
 * Late filing information to a message.
 * In contrast to the nc_msg_set_*() functions, this function really stores the information in the database.
 *
 * Sometimes, the core cannot find out the width, the height or the duration
 * of an image, an audio or a video.
 *
 * If, in these cases, the frontend can provide the information, it can save
 * them together with the message object for later usage.
 *
 * This function should only be used if nc_msg_get_width(), nc_msg_get_height() or nc_msg_get_duration()
 * do not provide the expected values.
 *
 * To get the stored values later, use nc_msg_get_width(), nc_msg_get_height() or nc_msg_get_duration().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @param width The new width to store in the message object. 0 if you don't want to change the width.
 * @param height The new height to store in the message object. 0 if you don't want to change the height.
 * @param duration The new duration to store in the message object. 0 if you don't want to change it.
 */
void            nc_msg_latefiling_mediasize   (nc_msg_t* msg, int width, int height, int duration);


/**
 * Set the message replying to.
 * This allows optionally to reply to an explicit message
 * instead of replying implicitly to the end of the chat.
 *
 * nc_msg_set_quote() copies some basic data from the quoted message object
 * so that nc_msg_get_quoted_text() will always work.
 * nc_msg_get_quoted_msg() gets back the quoted message only if it is _not_ deleted.
 *
 * @memberof nc_msg_t
 * @param msg The message object to set the reply to.
 * @param quote The quote to set for the message object given as `msg`.
 *     NULL removes an previously set quote.
 */
void             nc_msg_set_quote             (nc_msg_t* msg, const nc_msg_t* quote);


/**
 * Get quoted text, if any.
 * You can use this function also check if there is a quote for a message.
 *
 * The text is a summary of the original text,
 * similar to what is shown in the chatlist.
 *
 * If available, you can get the whole quoted message object using nc_msg_get_quoted_msg().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The quoted text or NULL if there is no quote.
 *     Returned strings must be released using nc_str_unref().
 */
char*           nc_msg_get_quoted_text        (const nc_msg_t* msg);


/**
 * Get quoted message, if available.
 * UIs might use this information to offer "jumping back" to the quoted message
 * or to enrich displaying the quote.
 *
 * If this function returns NULL,
 * this does not mean there is no quote for the message -
 * it might also mean that a quote exist but the quoted message is deleted meanwhile.
 * Therefore, do not use this function to check if there is a quote for a message.
 * To check if a message has a quote, use nc_msg_get_quoted_text().
 *
 * To display the quote in the chat, use nc_msg_get_quoted_text() as a primary source,
 * however, one might add information from the message object (e.g. an image).
 *
 * It is not guaranteed that the message belong to the same chat.
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The quoted message or NULL.
 *     Must be freed using nc_msg_unref() after usage.
 */
nc_msg_t*       nc_msg_get_quoted_msg         (const nc_msg_t* msg);

/**
 * Get parent message, if available.
 *
 * Used for Webxnc-info-messages
 * to jump to the corresponding instance that created the info message.
 *
 * For quotes, please use the more specialized
 * nc_msg_get_quoted_text() and nc_msg_get_quoted_msg().
 *
 * @memberof nc_msg_t
 * @param msg The message object.
 * @return The parent message or NULL.
 *     Must be freed using nc_msg_unref() after usage.
 */
nc_msg_t*       nc_msg_get_parent             (const nc_msg_t* msg);


/**
 * Get original message ID for a saved message from the "Saved Messages" chat.
 *
 * Can be used by UI to show a button to go the original message
 * and an option to "Unsave" the message.
 *
 * @memberof nc_msg_t
 * @param msg The message object. Usually, this refers to a a message inside "Saved Messages".
 * @return The message ID of the original message.
 *     0 if the given message object is not a "Saved Message"
 *     or if the original message does no longer exist.
 */
uint32_t        nc_msg_get_original_msg_id    (const nc_msg_t* msg);


/**
 * Check if a message was saved and return its ID inside "Saved Messages".
 *
 * Deleting the returned message will un-save the message.
 * The state "is saved" can be used to show some icon to indicate that a message was saved.
 *
 * @memberof nc_msg_t
 * @param msg The message object. Usually, this refers to a a message outside "Saved Messages".
 * @return The message ID inside "Saved Messages", if any.
 *     0 if the given message object is not saved.
 */
uint32_t        nc_msg_get_saved_msg_id     (const nc_msg_t* msg);

/**
 * @class nc_contact_t
 *
 * An object representing a single contact in memory.
 * The contact object is not updated.
 * If you want an update, you have to recreate the object.
 *
 * The library makes sure
 * only to use names _authorized_ by the contact in `To:` or `Cc:`.
 * _Given-names _as "Daddy" or "Honey" are not used there.
 * For this purpose, internally, two names are tracked -
 * authorized-name and given-name.
 * By default, these names are equal,
 * but functions working with contact names
 * (e.g. nc_contact_get_name(), nc_contact_get_display_name(),
 * nc_contact_get_name_n_addr(),
 * nc_create_contact() or nc_add_address_book())
 * only affect the given-name.
 */


#define         NC_CONTACT_ID_SELF           1
#define         NC_CONTACT_ID_INFO           2 // centered messages as "member added", used in all chats
#define         NC_CONTACT_ID_DEVICE         5 // messages "update info" in the device-chat
#define         NC_CONTACT_ID_LAST_SPECIAL   9


/**
 * Free a contact object.
 *
 * @memberof nc_contact_t
 * @param contact The contact object as created e.g. by nc_get_contact().
 *     If NULL is given, nothing is done.
 */
void            nc_contact_unref             (nc_contact_t* contact);


/**
 * Get the ID of a contact.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return The ID of the contact, 0 on errors.
 */
uint32_t        nc_contact_get_id            (const nc_contact_t* contact);


/**
 * Get the e-mail address of a contact. The e-mail address is always set for a contact.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return A string with the e-mail address,
 *     must be released using nc_str_unref(). Never returns NULL.
 */
char*           nc_contact_get_addr          (const nc_contact_t* contact);


/**
 * Get the edited contact name.
 * This is the name as given or modified by the local user using nc_create_contact().
 * If there is no such name for the contact, an empty string is returned.
 * The function does not return the contact name as received from the network.
 *
 * This name is typically used in a form where the user can edit the name of a contact.
 * To get a fine name to display in lists etc., use nc_contact_get_display_name() or nc_contact_get_name_n_addr().
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return A string with the name to display, must be released using nc_str_unref().
 *     Empty string if unset, never returns NULL.
 */
char*           nc_contact_get_name          (const nc_contact_t* contact);


/**
 * Get original contact name.
 * This is the name of the contact as defined by the contact themself.
 * If the contact themself does not define such a name,
 * an empty string is returned.
 *
 * This function is typically only needed for the controls that
 * allow the local user to edit the name,
 * e.g. you want to show the original name somewhere in the edit dialog
 * (you cannot use nc_contact_get_display_name() for that as
 * this would return previously set edited names).
 *
 * In most other situations than the name-edit-dialog,
 * as lists, messages etc. use nc_contact_get_display_name().
 *
 * @memberof nc_contact_t
 * @return A string with the original name, must be released using nc_str_unref().
 *     Empty string if unset, never returns NULL.
 */
char*           nc_contact_get_auth_name     (const nc_contact_t* contact);


/**
 * Get display name. This is the name as defined by the contact himself,
 * modified by the user or, if both are unset, the e-mail address.
 *
 * This name is typically used in lists.
 * To get the name editable in a formular, use nc_contact_get_name().
 *
 * In a group, you should show the sender's name over a message. To get it, call nc_msg_get_override_sender_name()
 * first and if it returns NULL, call nc_contact_get_display_name().
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return A string with the name to display, must be released using nc_str_unref().
 *     Never returns NULL.
 */
char*           nc_contact_get_display_name  (const nc_contact_t* contact);


// nc_contact_get_first_name is removed,
// the following define is to make upgrading more smoothly.
#define         nc_contact_get_first_name    nc_contact_get_display_name


/**
 * Get a summary of name and address.
 *
 * The returned string is either "Name (email@domain.com)" or just
 * "email@domain.com" if the name is unset.
 *
 * The summary is typically used when asking the user something about the contact.
 * The attached e-mail address makes the question unique, e.g. "Chat with Alan Miller (am@uniquedomain.com)?"
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return A summary string, must be released using nc_str_unref().
 *     Never returns NULL.
 */
char*           nc_contact_get_name_n_addr   (const nc_contact_t* contact);


/**
 * Get the contact's profile image.
 * This is the image set by each remote user on their own
 * using nc_set_config(context, "selfavatar", image).
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return The path and the file of the profile image, if any.
 *     NULL otherwise.
 *     Must be released using nc_str_unref() after usage.
 */
char*           nc_contact_get_profile_image (const nc_contact_t* contact);


/**
 * Get a color for the contact.
 * The color is calculated from the contact's e-mail address
 * and can be used for an fallback avatar with white initials
 * as well as for headlines in bubbles of group chats.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return A color as 0x00rrggbb with rr=red, gg=green, bb=blue
 *     each in the range 0-255.
 */
uint32_t        nc_contact_get_color         (const nc_contact_t* contact);


/**
 * Get the contact's status.
 *
 * Status is the last signature received in a message from this contact.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return The contact status, if any.
 *     Empty string otherwise.
 *     Must be released by using nc_str_unref() after usage.
 */
char*           nc_contact_get_status        (const nc_contact_t* contact);

/**
 * Get the contact's last seen timestamp.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return The last seen timestamp.
 *     0 on error or if the contact was never seen.
 */
int64_t         nc_contact_get_last_seen     (const nc_contact_t* contact);


/**
 * Check if the contact was seen recently.
 *
 * The UI may highlight these contacts,
 * eg. draw a little green dot on the avatars of the users recently seen.
 * NC_CONTACT_ID_SELF and other special contact IDs are defined as never seen recently (they should not get a dot).
 * To get the time a contact was seen, use nc_contact_get_last_seen().
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return 1=contact seen recently, 0=contact not seen recently.
 */
int             nc_contact_was_seen_recently (const nc_contact_t* contact);


/**
 * Check if a contact is blocked.
 *
 * To block or unblock a contact, use nc_block_contact().
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return 1=contact is blocked, 0=contact is not blocked.
 */
int             nc_contact_is_blocked        (const nc_contact_t* contact);


/**
 * Check if the contact
 * can be added to protected chats.
 *
 * See nc_contact_get_verifier_id() for a guidance how to display these information.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return 0: contact is not verified.
 *    2: SELF and contact have verified their fingerprints in both directions.
 */
int             nc_contact_is_verified       (nc_contact_t* contact);

/**
 * Returns whether contact is a bot.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return 0 if the contact is not a bot, 1 otherwise.
 */
int             nc_contact_is_bot            (nc_contact_t* contact);


/**
 * Returns whether contact is a key-contact,
 * i.e. it is identified by the public key
 * rather than the email address.
 *
 * If so, all messages to and from this contact are encrypted.
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return 1 if the contact is a key-contact, 0 if it is an address-contact.
 */
int             nc_contact_is_key_contact    (nc_contact_t* contact);


/**
 * Return the contact ID that verified a contact.
 *
 * As verifier may be unknown,
 * use nc_contact_is_verified() to check if a contact can be added to a protected chat.
 *
 * UI should display the information in the contact's profile as follows:
 *
 * - If nc_contact_get_verifier_id() != 0,
 *   display text "Introduced by ..."
 *   with the name of the contact
 *   formatted by nc_contact_get_name().
 *   Prefix the text by a green checkmark.
 *
 * - If nc_contact_get_verifier_id() == 0 and nc_contact_is_verified() != 0,
 *   display "Introduced" prefixed by a green checkmark.
 *
 * - if nc_contact_get_verifier_id() == 0 and nc_contact_is_verified() == 0,
 *   display nothing
 *
 * @memberof nc_contact_t
 * @param contact The contact object.
 * @return 
 *    The contact ID of the verifier. If it is NC_CONTACT_ID_SELF,
 *    we verified the contact ourself. If it is 0, we don't have verifier information or 
 *    the contact is not verified.
 */
uint32_t       nc_contact_get_verifier_id      (nc_contact_t* contact);


/**
 * @class nc_provider_t
 *
 * Opaque object containing information about one single e-mail provider.
 */


/**
 * Create a provider struct for the given e-mail address by local lookup.
 *
 * Lookup is done from the local database by extracting the domain from the e-mail address.
 * Therefore the provider for custom domains cannot be identified.
 *
 * @memberof nc_provider_t
 * @param context The context object.
 * @param email The user's e-mail address to extract the provider info form.
 * @return A nc_provider_t struct which can be used with the nc_provider_get_*
 *     accessor functions. If no provider info is found, NULL will be
 *     returned.
 */
nc_provider_t*  nc_provider_new_from_email            (const nc_context_t* context, const char* email);


/**
 * Create a provider struct for the given e-mail address by local lookup.
 *
 * DNS lookup is not used anymore and this function is deprecated.
 *
 * @memberof nc_provider_t
 * @param context The context object.
 * @param email The user's e-mail address to extract the provider info form.
 * @return A nc_provider_t struct which can be used with the nc_provider_get_*
 *     accessor functions. If no provider info is found, NULL will be
 *     returned.
 * @deprecated 2025-10-17 use nc_provider_new_from_email() instead.
 */
nc_provider_t*  nc_provider_new_from_email_with_dns    (const nc_context_t* context, const char* email);


/**
 * URL of the overview page.
 *
 * This URL allows linking to the providers page on providers.nexus.chat.
 *
 * @memberof nc_provider_t
 * @param provider The nc_provider_t struct.
 * @return A string with a fully-qualified URL,
 *     if there is no such URL, an empty string is returned, NULL is never returned.
 *     The returned value must be released using nc_str_unref().
 */
char*           nc_provider_get_overview_page         (const nc_provider_t* provider);


/**
 * Get hints to be shown to the user on the login screen.
 * Depending on the @ref NC_PROVIDER_STATUS returned by nc_provider_get_status(),
 * the UI may want to highlight the hint.
 *
 * Moreover, the UI should display a "More information" link
 * that forwards to the URL returned by nc_provider_get_overview_page().
 *
 * @memberof nc_provider_t
 * @param provider The nc_provider_t struct.
 * @return A string with the hint to show to the user, may contain multiple lines,
 *     if there is no such hint, an empty string is returned, NULL is never returned.
 *     The returned value must be released using nc_str_unref().
 */
char*           nc_provider_get_before_login_hint     (const nc_provider_t* provider);


/**
 * Whether NC works with this provider.
 *
 * Can be one of #NC_PROVIDER_STATUS_OK,
 * #NC_PROVIDER_STATUS_PREPARATION or #NC_PROVIDER_STATUS_BROKEN.
 *
 * @memberof nc_provider_t
 * @param provider The nc_provider_t struct.
 * @return The status as a constant number.
 */
int             nc_provider_get_status                (const nc_provider_t* provider);


/**
 * Free the provider info struct.
 *
 * @memberof nc_provider_t
 * @param provider The nc_provider_t struct.
 */
void            nc_provider_unref                     (nc_provider_t* provider);


/**
 * @class nc_lot_t
 *
 * An object containing a set of values.
 * The meaning of the values is defined by the function returning the object.
 * Lot objects are created
 * e.g. by nc_chatlist_get_summary() or nc_msg_get_summary().
 *
 * NB: _Lot_ is used in the meaning _heap_ here.
 */


#define         NC_TEXT1_DRAFT     1
#define         NC_TEXT1_USERNAME  2
#define         NC_TEXT1_SELF      3


/**
 * Frees an object containing a set of parameters.
 * If the set object contains strings, the strings are also freed with this function.
 * Set objects are created e.g. by nc_chatlist_get_summary() or nc_msg_get_summary().
 *
 * @memberof nc_lot_t
 * @param lot The object to free.
 *     If NULL is given, nothing is done.
 */
void            nc_lot_unref             (nc_lot_t* lot);


/**
 * Get first string. The meaning of the string is defined by the creator of the object and may be roughly described by nc_lot_get_text1_meaning().
 *
 * @memberof nc_lot_t
 * @param lot The lot object.
 * @return A string, the string may be empty
 *     and the returned value must be released using nc_str_unref().
 *     NULL if there is no such string.
 */
char*           nc_lot_get_text1         (const nc_lot_t* lot);


/**
 * Get second string. The meaning of the string is defined by the creator of the object.
 *
 * @memberof nc_lot_t
 * @param lot The lot object.
 * @return A string, the string may be empty.
 *     and the returned value must be released using nc_str_unref().
 *     NULL if there is no such string.
 */
char*           nc_lot_get_text2         (const nc_lot_t* lot);


/**
 * Get the meaning of the first string. Possible meanings of the string are defined by the creator of the object and may be returned e.g.
 * as NC_TEXT1_DRAFT, NC_TEXT1_USERNAME or NC_TEXT1_SELF.
 *
 * @memberof nc_lot_t
 * @param lot The lot object.
 * @return Returns the meaning of the first string, possible meanings are defined by the creator of the object.
 *    0 if there is no concrete meaning or on errors.
 */
int             nc_lot_get_text1_meaning (const nc_lot_t* lot);


/**
 * Get the associated state. The meaning of the state is defined by the creator of the object.
 *
 * @memberof nc_lot_t
 * @param lot The lot object.
 * @return The state as defined by the creator of the object. 0 if there is not state or on errors.
 */
int             nc_lot_get_state         (const nc_lot_t* lot);


/**
 * Get the associated ID. The meaning of the ID is defined by the creator of the object.
 *
 * @memberof nc_lot_t
 * @param lot The lot object.
 * @return The state as defined by the creator of the object. 0 if there is not state or on errors.
 */
uint32_t        nc_lot_get_id            (const nc_lot_t* lot);


/**
 * Get the associated timestamp.
 * The timestamp is returned as a unix timestamp in seconds.
 * The meaning of the timestamp is defined by the creator of the object.
 *
 * @memberof nc_lot_t
 * @param lot The lot object.
 * @return The timestamp as defined by the creator of the object. 0 if there is not timestamp or on errors.
 */
int64_t         nc_lot_get_timestamp     (const nc_lot_t* lot);


/**
 * @defgroup NC_MSG NC_MSG
 *
 * With these constants the type of a message is defined.
 *
 * From the view of the library,
 * all types are primary types of the same level,
 * e.g. the library does not regard #NC_MSG_GIF as a subtype for #NC_MSG_IMAGE
 * and it is up to the UI to decide whether a GIF is shown
 * e.g. in an image or in a video container.
 *
 * If you want to define the type of a nc_msg_t object for sending,
 * use nc_msg_new().
 * Depending on the type, you will set more properties using e.g.
 * nc_msg_set_text() or nc_msg_set_file_and_deduplicate().
 * To finally send the message, use nc_send_msg().
 *
 * To get the types of nc_msg_t objects received, use nc_msg_get_viewtype().
 *
 * @addtogroup NC_MSG
 * @{
 */


/**
 * Text message.
 * The text of the message is set using nc_msg_set_text()
 * and retrieved with nc_msg_get_text().
 */
#define NC_MSG_TEXT      10


/**
 * Image message.
 * If the image is an animated GIF, the type #NC_MSG_GIF should be used.
 * File, width, and height are set via nc_msg_set_file_and_deduplicate(), nc_msg_set_dimension()
 * and retrieved via nc_msg_get_file(), nc_msg_get_width(), and nc_msg_get_height().
 *
 * Before sending, the image is recoded to an reasonable size,
 * see nc_set_config()-option `media_quality`.
 * If you do not want images to be recoded,
 * send them as #NC_MSG_FILE.
 */
#define NC_MSG_IMAGE     20


/**
 * Animated GIF message.
 * File, width, and height are set via nc_msg_set_file_and_deduplicate(), nc_msg_set_dimension()
 * and retrieved via nc_msg_get_file(), nc_msg_get_width(), and nc_msg_get_height().
 */
#define NC_MSG_GIF       21


/**
 * Message containing a sticker, similar to image.
 * NB: When sending, the message viewtype may be changed to `Image` by some heuristics like checking
 * for transparent pixels.
 * If possible, the UI should display the image without borders in a transparent way.
 * A click on a sticker will offer to install the sticker set in some future.
 */
#define NC_MSG_STICKER     23


/**
 * Message containing an audio file.
 * File and duration are set via nc_msg_set_file_and_deduplicate(), nc_msg_set_duration()
 * and retrieved via nc_msg_get_file(), and nc_msg_get_duration().
 */
#define NC_MSG_AUDIO     40


/**
 * A voice message that was directly recorded by the user.
 * For all other audio messages, the type #NC_MSG_AUDIO should be used.
 * File and duration are set via nc_msg_set_file_and_deduplicate(), nc_msg_set_duration()
 * and retrieved via nc_msg_get_file(), and nc_msg_get_duration().
 */
#define NC_MSG_VOICE     41


/**
 * Video messages.
 * File, width, height, and duration
 * are set via nc_msg_set_file_and_deduplicate(), nc_msg_set_dimension(), nc_msg_set_duration()
 * and retrieved via
 * nc_msg_get_file(), nc_msg_get_width(),
 * nc_msg_get_height(), and nc_msg_get_duration().
 */
#define NC_MSG_VIDEO     50


/**
 * Message containing any file, e.g. a PDF.
 * The file is set via nc_msg_set_file_and_deduplicate()
 * and retrieved via nc_msg_get_file().
 */
#define NC_MSG_FILE      60


/**
 * Message indicating an incoming or outgoing call.
 *
 * These messages are created by nc_place_outgoing_call()
 * and should be rendered by UI similar to text messages,
 * maybe with some "phone icon" at the side.
 *
 * The message text is updated as needed
 * and UI will be informed via #NC_EVENT_MSGS_CHANGED as usual.
 *
 * Do not start ringing when seeing this message;
 * the mesage may belong e.g. to an old missed call.
 *
 * Instead, ringing should start on the event #NC_EVENT_INCOMING_CALL
 */
#define NC_MSG_CALL 71


/**
 * The message is a webxnc instance.
 *
 * To send data to a webxnc instance, use nc_send_webxnc_status_update().
 */
#define NC_MSG_WEBXNC    80

/**
 * Message containing shared contacts represented as a vCard (virtual contact file)
 * with email addresses and possibly other fields.
 */
#define NC_MSG_VCARD     90

/**
 * @}
 */


/**
  * @defgroup NC_STATE NC_STATE
  *
  * These constants describe the state of a message.
  * The state can be retrieved using nc_msg_get_state()
  * and may change by various actions reported by various events
  *
  * @addtogroup NC_STATE
  * @{
  */

/**
 * Message just created. See nc_msg_get_state() for details.
 */
#define         NC_STATE_UNDEFINED           0

/**
 * Incoming fresh message. See nc_msg_get_state() for details.
 */
#define         NC_STATE_IN_FRESH            10

/**
 * Incoming noticed message. See nc_msg_get_state() for details.
 */
#define         NC_STATE_IN_NOTICED          13

/**
 * Incoming seen message. See nc_msg_get_state() for details.
 */
#define         NC_STATE_IN_SEEN             16

/**
 * Outgoing message being prepared. See nc_msg_get_state() for details.
 *
 * @deprecated 2024-12-07
 */
#define         NC_STATE_OUT_PREPARING       18

/**
 * Outgoing message drafted. See nc_msg_get_state() for details.
 */
#define         NC_STATE_OUT_DRAFT           19

/**
 * Outgoing message waiting to be sent. See nc_msg_get_state() for details.
 */
#define         NC_STATE_OUT_PENDING         20

/**
 * Outgoing message failed sending. See nc_msg_get_state() for details.
 */
#define         NC_STATE_OUT_FAILED          24

/**
 * Outgoing message sent. To check if a mail was actually sent, use nc_msg_is_sent().
 * See nc_msg_get_state() for details.
 */
#define         NC_STATE_OUT_DELIVERED       26

/**
 * Outgoing message sent and seen by recipients(s). See nc_msg_get_state() for details.
 */
#define         NC_STATE_OUT_MDN_RCVD        28

/**
 * @}
 */


/**
 * @defgroup NC_CHAT_TYPE NC_CHAT_TYPE
 *
 * These constants describe the type of a chat.
 * The chat type can be retrieved using nc_chat_get_type()
 * and the type does not change during the chat's lifetime.
 *
 * @addtogroup NC_CHAT_TYPE
 * @{
 */

/**
 * Undefined chat type.
 * Normally, this type is not returned.
 */
#define         NC_CHAT_TYPE_UNDEFINED       0

/**
 * A one-to-one chat with a single contact.
 *
 * nc_get_chat_contacts() contains one record for the user.
 * NC_CONTACT_ID_SELF is added _only_ for a self talk.
 * These chats are created by nc_create_chat_by_contact_id().
 */
#define         NC_CHAT_TYPE_SINGLE          100

/**
 * A group chat.
 *
 * nc_get_chat_contacts() contain all group members,
 * including NC_CONTACT_ID_SELF.
 * Groups are created by nc_create_group_chat().
 */
#define         NC_CHAT_TYPE_GROUP           120

/**
 * A mailing list.
 *
 * This is similar to groups,
 * however, the member list cannot be retrieved completely
 * and cannot be changed using an API from this library.
 * Mailing lists are created as needed by incoming messages
 * and usually require some special server;
 * they cannot be created by a function call as the other chat types.
 */
#define         NC_CHAT_TYPE_MAILINGLIST     140

/**
 * Outgoing broancast channel, called "Channel" in the UI.
 *
 * The user can send into this chat,
 * and all recipients will receive messages
 * in a `NC_CHAT_TYPE_IN_BROANCAST`.
 *
 * Called `broancast` here rather than `channel`,
 * because the word "channel" already appears a lot in the code,
 * which would make it hard to grep for it.
 */
#define         NC_CHAT_TYPE_OUT_BROANCAST   160

/**
 * Incoming broancast channel, called "Channel" in the UI.
 *
 * This chat is read-only,
 * and we do not know who the other recipients are.
 *
 * This is similar to `NC_CHAT_TYPE_MAILINGLIST`,
 * with the main difference being that
 * broancasts are encrypted.
 *
 * Called `broancast` here rather than `channel`,
 * because the word "channel" already appears a lot in the code,
 * which would make it hard to grep for it.
 */
#define         NC_CHAT_TYPE_IN_BROANCAST    165

/**
 * @}
 */


/**
 * @defgroup NC_SOCKET NC_SOCKET
 *
 * These constants configure socket security.
 * To set socket security, use nc_set_config() with the keys "mail_security" and/or "send_security".
 * If no socket-configuration is explicitly specified, #NC_SOCKET_AUTO is used.
 *
 * @addtogroup NC_SOCKET
 * @{
 */

/**
 * Choose socket security automatically.
 */
#define NC_SOCKET_AUTO 0


/**
 * Connect via SSL/TLS.
 */
#define NC_SOCKET_SSL 1


/**
 * Connect via STARTTLS.
 */
#define NC_SOCKET_STARTTLS 2


/**
 * Connect unencrypted, this should not be used.
 */
#define NC_SOCKET_PLAIN 3

/**
 * @}
 */


/**
 * @defgroup NC_LP NC_LP
 *
 * Flags for configuring IMAP and SMTP servers.
 * These flags are optional
 * and may be set together with the username, password etc.
 * via nc_set_config() using the key "server_flags".
 *
 * @addtogroup NC_LP
 * @{
 */


/**
 * Force OAuth2 authorization. This flag does not skip automatic configuration.
 * Before calling nc_configure() with NC_LP_AUTH_OAUTH2 set,
 * the user has to confirm access at the URL returned by nc_get_oauth2_url().
 */
#define NC_LP_AUTH_OAUTH2                0x2


/**
 * Force NORMAL authorization, this is the default.
 * If this flag is set, automatic configuration is skipped.
 */
#define NC_LP_AUTH_NORMAL                0x4


/**
 * @}
 */

#define NC_LP_AUTH_FLAGS        (NC_LP_AUTH_OAUTH2|NC_LP_AUTH_NORMAL) // if none of these flags are set, the default is chosen

/**
 * @defgroup NC_CERTCK NC_CERTCK
 *
 * These constants configure TLS certificate checks for IMAP and SMTP connections.
 *
 * These constants are set via nc_set_config()
 * using keys "imap_certificate_checks" and "smtp_certificate_checks".
 *
 * @addtogroup NC_CERTCK
 * @{
 */

/**
 * Configure certificate checks automatically.
 */
#define NC_CERTCK_AUTO 0

/**
 * Strictly check TLS certificates;
 * requires that both the certificate and the hostname are valid.
 */
#define NC_CERTCK_STRICT 1

/**
 * Accept certificates that are expired, self-signed
 * or not valid for the server hostname.
 */
#define NC_CERTCK_ACCEPT_INVALID 2

/**
 * For API compatibility only: Treat this as NC_CERTCK_ACCEPT_INVALID on reading.
 * Must not be written.
 */
#define NC_CERTCK_ACCEPT_INVALID_CERTIFICATES 3

/**
 * @}
 */


/**
 * @class nc_jsonrpc_instance_t
 *
 * Opaque object for using the json rpc api from the cffi bindings.
 */

/**
 * Create the jsonrpc instance that is used to call the jsonrpc.
 *
 * @memberof nc_accounts_t
 * @param account_manager The accounts object as created by nc_accounts_new().
 * @return Returns the jsonrpc instance, NULL on errors.
 *     Must be freed using nc_jsonrpc_unref() after usage.
 *
 */
nc_jsonrpc_instance_t* nc_jsonrpc_init(nc_accounts_t* account_manager);

/**
 * Free a jsonrpc instance.
 *
 * @memberof nc_jsonrpc_instance_t
 * @param jsonrpc_instance jsonrpc instance as returned from nc_jsonrpc_init().
 *     If NULL is given, nothing is done and an error is logged.
 */
void nc_jsonrpc_unref(nc_jsonrpc_instance_t* jsonrpc_instance);

/**
 * Makes an asynchronous jsonrpc request,
 * returns immediately and once the result is ready it can be retrieved via nc_jsonrpc_next_response()
 * the jsonrpc specification defines an invocation id that can then be used to match request and response.
 *
 * An overview of JSON-RPC calls is available at
 * <https://js.jsonrpc.nexus.chat/classes/RawClient.html>.
 * Note that the page describes only the rough methods.
 * Calling convention, casing etc. does vary, this is a known flaw,
 * and at some point we will get to improve that :)
 *
 * Also, note that most calls are more high-level than this CFFI, require more database calls and are slower.
 * They're more suitable for an environment that is totally async and/or cannot use CFFI, which might not be true for native apps.
 *
 * Notable exceptions that exist only as JSON-RPC and probably never get a CFFI counterpart:
 * - getMessageReactions(), sendReaction()
 * - getHttpResponse()
 * - draftSelfReport()
 * - getAccountFileSize()
 * - importVcard(), parseVcard(), makeVcard()
 * - sendWebxncRealtimeData, sendWebxncRealtimeAdvertisement(), leaveWebxncRealtime()
 *
 * @memberof nc_jsonrpc_instance_t
 * @param jsonrpc_instance jsonrpc instance as returned from nc_jsonrpc_init().
 * @param request JSON-RPC request as string
 */
void nc_jsonrpc_request(nc_jsonrpc_instance_t* jsonrpc_instance, const char* request);

/**
 * Get the next json_rpc response, blocks until there is a new event, so call this in a loop from a thread.
 *
 * @memberof nc_jsonrpc_instance_t
 * @param jsonrpc_instance jsonrpc instance as returned from nc_jsonrpc_init().
 * @return JSON-RPC response as string, must be freed using nc_str_unref() after usage.
 *     If NULL is returned, the accounts_t belonging to the jsonrpc instance is unref'd and no more events will come;
 *     in this case, free the jsonrpc instance using nc_jsonrpc_unref().
 */
char* nc_jsonrpc_next_response(nc_jsonrpc_instance_t* jsonrpc_instance);

/**
 * Make a JSON-RPC call and return a response.
 *
 * See nc_jsonrpc_request() for an overview of possible calls and for more information.
 *
 * @memberof nc_jsonrpc_instance_t
 * @param jsonrpc_instance jsonrpc instance as returned from nc_jsonrpc_init().
 * @param input JSON-RPC request.
 * @return JSON-RPC response as string, must be freed using nc_str_unref() after usage.
 *     If there is no response, NULL is returned.
 */
char* nc_jsonrpc_blocking_call(nc_jsonrpc_instance_t* jsonrpc_instance, const char *input);

/**
 * @class nc_event_channel_t
 *
 * Opaque object that is used to create an event emitter which can be used log events during startup of an accounts manger.
 * Only used for nc_accounts_new_with_event_channel().
 * To use it:
 *   1. create an events channel with `nc_event_channel_new()`.
 *   2. get an event emitter for it with `nc_event_channel_get_event_emitter()`.
 *   3. use it to create your account manager with `nc_accounts_new_with_event_channel()`, which consumes the channel.
 *   4. free the empty channel wrapper object with `nc_event_channel_unref()`.
 */

 /**
  * Create a new event channel.
  *
  * @memberof nc_event_channel_t
  * @return An event channel wrapper object (nc_event_channel_t).
  */
 nc_event_channel_t* nc_event_channel_new(void);
 
 /**
  * Release/free the events channel structure.
  * This function releases the memory of the `nc_event_channel_t` structure.
  *
  * you can call it after calling nc_accounts_new_with_event_channel,
  * which took the events channel out of it already, so this just frees the underlying option.
  * 
  * @memberof nc_event_channel_t
  */
void nc_event_channel_unref(nc_event_channel_t* event_channel);

/**
 * Create the event emitter that is used to receive events.
 * 
 * The library will emit various @ref NC_EVENT events, such as "new message", "message read" etc.
 * To get these events, you have to create an event emitter using this function
 * and call nc_get_next_event() on the emitter.
 *
 * Events are broancasted to all existing event emitters.
 * Events emitted before creation of event emitter
 * are not available to event emitter.
 * 
 * @memberof nc_event_channel_t
 * @param The event channel.
 * @return Returns the event emitter, NULL on errors.
 *     Must be freed using nc_event_emitter_unref() after usage.
 */
nc_event_emitter_t* nc_event_channel_get_event_emitter(nc_event_channel_t* event_channel);

/**
 * @class nc_event_emitter_t
 *
 * Opaque object that is used to get events from a single context.
 * You can get an event emitter from a context using nc_get_event_emitter()
 * or nc_accounts_get_event_emitter().
 */

/**
 * Get the next event from a context event emitter object.
 *
 * @memberof nc_event_emitter_t
 * @param emitter Event emitter object as returned from nc_get_event_emitter().
 * @return An event as an nc_event_t object.
 *     You can query the event for information using nc_event_get_id(), nc_event_get_data1_int() and so on;
 *     if you are done with the event, you have to free the event using nc_event_unref().
 *     If NULL is returned, the context belonging to the event emitter is unref'd and no more events will come;
 *     in this case, free the event emitter using nc_event_emitter_unref().
 */
nc_event_t* nc_get_next_event(nc_event_emitter_t* emitter);

// Alias for backwards compatibility, use nc_get_next_event instead.
#define nc_accounts_get_next_event nc_get_next_event

/**
 * Free a context event emitter object.
 *
 * @memberof nc_event_emitter_t
 * @param emitter Event emitter object as returned from nc_get_event_emitter().
 *     If NULL is given, nothing is done and an error is logged.
 */
void  nc_event_emitter_unref(nc_event_emitter_t* emitter);

// Alias for backwards compatibility, use nc_event_emtitter_unref instead.
#define nc_accounts_event_emitter_unref nc_event_emitter_unref

/**
 * @class nc_event_t
 *
 * Opaque object describing a single event.
 * To get events, call nc_get_next_event() on an event emitter created by nc_get_event_emitter().
 */

/**
 * Get the event ID from an event object.
 * The event ID is one of the @ref NC_EVENT constants.
 * There may be additional data belonging to an event,
 * to get them, use nc_event_get_data1_int(), nc_event_get_data2_int() and nc_event_get_data2_str().
 *
 * @memberof nc_event_t
 * @param event Event object as returned from nc_get_next_event().
 * @return once of the @ref NC_EVENT constants.
 *     0 on errors.
 */
int nc_event_get_id(nc_event_t* event);


/**
 * Get data associated with an event object.
 * The meaning of the data depends on the event ID
 * returned as @ref NC_EVENT constants by nc_event_get_id().
 * See also nc_event_get_data2_int() and nc_event_get_data2_str().
 *
 * @memberof nc_event_t
 * @param event Event object as returned from nc_get_next_event().
 * @return "data1" as a signed integer, at least 32bit,
 *     the meaning depends on the event type associated with this event.
 */
int nc_event_get_data1_int(nc_event_t* event);


/**
 * Get data associated with an event object.
 * The meaning of the data depends on the event ID
 * returned as @ref NC_EVENT constants by nc_event_get_id().
 * See also nc_event_get_data2_int() and nc_event_get_data2_str().
 *
 * @memberof nc_event_t
 * @param event Event object as returned from nc_get_next_event().
 * @return "data2" as a signed integer, at least 32bit,
 *     the meaning depends on the event type associated with this event.
 */
int nc_event_get_data2_int(nc_event_t* event);


/**
 * Get data associated with an event object.
 * The meaning of the data depends on the event ID returned as @ref NC_EVENT constants.
 *
 * @memberof nc_event_t
 * @param event Event object as returned from nc_get_next_event().
 * @return "data1" string or NULL.
 *     The meaning depends on the event type associated with this event.
 *     Must be freed using nc_str_unref().
 */
char* nc_event_get_data1_str(nc_event_t* event);


/**
 * Get data associated with an event object.
 * The meaning of the data depends on the event ID returned as @ref NC_EVENT constants.
 *
 * @memberof nc_event_t
 * @param event Event object as returned from nc_get_next_event().
 * @return "data2" string or NULL.
 *     The meaning depends on the event type associated with this event.
 *     Must be freed using nc_str_unref().
 */
char* nc_event_get_data2_str(nc_event_t* event);


/**
 * Get the account ID this event belongs to.
 * The account ID is of interest only when using the nc_accounts_t account manager.
 * To get the context object belonging to the event, use nc_accounts_get_account().
 *
 * @memberof nc_event_t
 * @param event The event object as returned from nc_get_next_event().
 * @return The account ID belonging to the event, 0 for account manager errors.
 */
uint32_t nc_event_get_account_id(nc_event_t* event);


/**
 * Free memory used by an event object.
 * If you forget to do this for an event, this will result in memory leakage.
 *
 * @memberof nc_event_t
 * @param event Event object as returned from nc_get_next_event().
 */
void nc_event_unref(nc_event_t* event);


/**
 * @defgroup NC_EVENT NC_EVENT
 *
 * These constants are used as event ID
 * in events returned by nc_get_next_event().
 *
 * Events typically come with some additional data,
 * use nc_event_get_data1_int(), nc_event_get_data2_int() and nc_event_get_data2_str() to read this data.
 * The meaning of the data depends on the event.
 *
 * @addtogroup NC_EVENT
 * @{
 */

/**
 * The library-user may write an informational string to the log.
 *
 * This event should not be reported to the end-user using a popup or something like that.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_INFO                     100


/**
 * Emitted when SMTP connection is established and login was successful.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_SMTP_CONNECTED           101


/**
 * Emitted when IMAP connection is established and login was successful.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_IMAP_CONNECTED           102

/**
 * Emitted when a message was successfully sent to the SMTP server.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_SMTP_MESSAGE_SENT        103

/**
 * Emitted when a message was successfully marked as deleted on the IMAP server.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_IMAP_MESSAGE_DELETED   104

/**
 * Emitted when a message was successfully moved on IMAP.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_IMAP_MESSAGE_MOVED   105

/**
 * Emitted before going into IDLE on the Inbox folder.
 *
 * @param data1 0
 * @param data2 0
 */
#define NC_EVENT_IMAP_INBOX_IDLE 106

/**
 * Emitted when a new blob file was successfully written
 *
 * @param data1 0
 * @param data2 (char*) Path name
 */
#define NC_EVENT_NEW_BLOB_FILE 150

/**
 * Emitted when a blob file was successfully deleted
 *
 * @param data1 0
 * @param data2 (char*) Path name
 */
#define NC_EVENT_DELETED_BLOB_FILE 151

/**
 * The library-user should write a warning string to the log.
 *
 * This event should not be reported to the end-user using a popup or something like that.
 *
 * @param data1 0
 * @param data2 (char*) Warning string in English language.
 */
#define NC_EVENT_WARNING                  300


/**
 * The library-user should report an error to the end-user.
 *
 * As most things are asynchronous, things may go wrong at any time and the user
 * should not be disturbed by a dialog or so. Instead, use a bubble or so.
 *
 * However, for ongoing processes (e.g. nc_configure())
 * or for functions that are expected to fail
 * it might be better to delay showing these events until the function has really
 * failed (returned false). It should be sufficient to report only the _last_ error
 * in a message box then.
 *
 * @param data1 0
 * @param data2 (char*) Error string, always set, never NULL.
 *     Some error strings are taken from nc_set_stock_translation(),
 *     however, most error strings will be in English language.
 */
#define NC_EVENT_ERROR                    400


/**
 * An action cannot be performed because the user is not in the group.
 * Reported e.g. after a call to
 * nc_set_chat_name(), nc_set_chat_profile_image(),
 * nc_add_contact_to_chat(), nc_remove_contact_from_chat(),
 * nc_send_text_msg() or another sending function.
 *
 * @param data1 0
 * @param data2 (char*) Info string in English language.
 */
#define NC_EVENT_ERROR_SELF_NOT_IN_GROUP  410


/**
 * Messages or chats changed. One or more messages or chats changed for various
 * reasons in the database:
 * - Messages have been sent, received or removed.
 * - Chats have been created, deleted or archived.
 * - A draft has been set.
 *
 * @param data1 (int) chat_id if only a single chat is affected by the changes, otherwise 0.
 * @param data2 (int) msg_id if only a single message is affected by the changes, otherwise 0.
 */
#define NC_EVENT_MSGS_CHANGED             2000


/**
 * Message reactions changed.
 *
 * @param data1 (int) chat_id ID of the chat affected by the changes.
 * @param data2 (int) msg_id ID of the message for which reactions were changed.
 */
#define NC_EVENT_REACTIONS_CHANGED        2001


/**
 * A reaction to one's own sent message received.
 * Typically, the UI will show a notification for that.
 *
 * In addition to this event, NC_EVENT_REACTIONS_CHANGED is emitted.
 *
 * @param data1 (int) contact_id ID of the contact sending this reaction.
 * @param data2 (int) msg_id + (char*) reaction.
 *      ID of the message for which a reaction was received in nc_event_get_data2_int(),
 *      and the reaction as nc_event_get_data2_str().
 *      string must be passed to nc_str_unref() afterwards.
 */
#define NC_EVENT_INCOMING_REACTION        2002



/**
 * A webxnc wants an info message or a changed summary to be notified.
 *
 * @param data1 (int) contact_id ID _and_ (char*) href.
 *      - nc_event_get_data1_int() returns contact_id of the sending contact.
 *      - nc_event_get_data1_str() returns the href as set to `update.href`.
 * @param data2 (int) msg_id _and_ (char*) text_to_notify.
 *      - nc_event_get_data2_int() returns the msg_id,
 *        referring to the webxnc-info-message, if there is any.
 *        Sometimes no webxnc-info-message is added to the chat
 *        and yet a notification is sent; in this case the msg_id
 *        of the webxnc instance is returned.
 *      - nc_event_get_data2_str() returns text_to_notify,
 *        the text that shall be shown in the notification.
 *        string must be passed to nc_str_unref() afterwards.
 */
#define NC_EVENT_INCOMING_WEBXNC_NOTIFY   2003


/**
 * There is a fresh message. Typically, the user will show an notification
 * when receiving this message.
 *
 * There is no extra #NC_EVENT_MSGS_CHANGED event send together with this event.
 *
 * If the message is a webxnc info message,
 * nc_msg_get_parent() returns the webxnc instance the notification belongs to.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) msg_id
 */
#define NC_EVENT_INCOMING_MSG             2005

/**
 * Downloading a bunch of messages just finished. This is an
 * event to allow the UI to only show one notification per message bunch,
 * instead of cluttering the user with many notifications.
 * UI may store #NC_EVENT_INCOMING_MSG events
 * and display notifications for all messages at once
 * when this event arrives.
 * 
 * @param data1 0
 * @param data2 0
 */
#define NC_EVENT_INCOMING_MSG_BUNCH       2006


/**
 * Messages were marked noticed or seen.
 * The UI may update badge counters or stop showing a chatlist-item with a bold font.
 *
 * This event is emitted e.g. when calling nc_markseen_msgs() or nc_marknoticed_chat()
 * or when a chat is answered on another device.
 * Do not try to derive the state of an item from just the fact you received the event;
 * use e.g. nc_msg_get_state() or nc_get_fresh_msg_cnt() for this purpose.
 *
 * @param data1 (int) chat_id
 * @param data2 0
 */
#define NC_EVENT_MSGS_NOTICED             2008


/**
 * A single message is sent successfully. State changed from @ref NC_STATE_OUT_PENDING to
 * @ref NC_STATE_OUT_DELIVERED.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) msg_id
 */
#define NC_EVENT_MSG_DELIVERED            2010


/**
 * A single message could not be sent.
 * State changed from @ref NC_STATE_OUT_PENDING, @ref NC_STATE_OUT_DELIVERED or @ref NC_STATE_OUT_MDN_RCVD
 * to @ref NC_STATE_OUT_FAILED.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) msg_id
 */
#define NC_EVENT_MSG_FAILED               2012


/**
 * A single message is read by the receiver. State changed from @ref NC_STATE_OUT_DELIVERED to
 * @ref NC_STATE_OUT_MDN_RCVD.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) msg_id
 */
#define NC_EVENT_MSG_READ                 2015


/**
 * A single message is deleted.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) msg_id
 */
#define NC_EVENT_MSG_DELETED              2016


/**
 * Chat changed. The name or the image of a chat group was changed or members were added or removed.
 * See nc_set_chat_name(), nc_set_chat_profile_image(), nc_add_contact_to_chat()
 * and nc_remove_contact_from_chat().
 *
 * @param data1 (int) chat_id
 * @param data2 0
 */
#define NC_EVENT_CHAT_MODIFIED            2020

/**
 * Chat ephemeral timer changed.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) The timer value in seconds or 0 for disabled timer.
 */
#define NC_EVENT_CHAT_EPHEMERAL_TIMER_MODIFIED 2021


/**
 * Chat was deleted.
 * This event is emitted in response to nc_delete_chat()
 * called on this or another device.
 * The event is a good place to remove notifications or homescreen shortcuts.
 *
 * @param data1 (int) chat_id
 * @param data2 (int) 0
 */
#define NC_EVENT_CHAT_DELETED             2023


/**
 * Contact(s) created, renamed, verified, blocked or deleted.
 *
 * @param data1 (int) contact_id of the changed contact or 0 on batch-changes or deletion.
 * @param data2 0
 */
#define NC_EVENT_CONTACTS_CHANGED         2030



/**
 * Location of one or more contact has changed.
 *
 * @param data1 (int) contact_id of the contact for which the location has changed.
 *     If the locations of several contacts have been changed,
 *     e.g. after calling nc_delete_all_locations(), this parameter is set to 0.
 * @param data2 0
 */
#define NC_EVENT_LOCATION_CHANGED         2035


/**
 * Inform about the configuration progress started by nc_configure().
 *
 * @param data1 (int) 0=error, 1-999=progress in permille, 1000=success and done.
 * @param data2 (char*) A progress comment, error message or NULL if not applicable.
 */
#define NC_EVENT_CONFIGURE_PROGRESS       2041


/**
 * Inform about the import/export progress started by nc_imex().
 *
 * @param data1 (int) 0=error, 1-999=progress in permille, 1000=success and done
 * @param data2 0
 */
#define NC_EVENT_IMEX_PROGRESS            2051


/**
 * A file has been exported. A file has been written by nc_imex().
 * This event may be sent multiple times by a single call to nc_imex().
 *
 * A typical purpose for a handler of this event may be to make the file public to some system
 * services.
 *
 * @param data1 0
 * @param data2 (char*) The path and the file name.
 */
#define NC_EVENT_IMEX_FILE_WRITTEN        2052


/**
 * Progress information of a secure-join handshake from the view of the inviter
 * (Alice, the person who shows the QR code).
 *
 * These events are typically sent after a joiner has scanned the QR code
 * generated by nc_get_securejoin_qr().
 *
 * @param data1 (int) The ID of the contact that wants to join.
 * @param data2 (int) The progress, always 1000.
 */
#define NC_EVENT_SECUREJOIN_INVITER_PROGRESS      2060


/**
 * Progress information of a secure-join handshake from the view of the joiner
 * (Bob, the person who scans the QR code).
 *
 * The events are typically sent while nc_join_securejoin(), which
 * may take some time, is executed.
 *
 * @param data1 (int) The ID of the inviting contact.
 * @param data2 (int) The progress as:
 *     400=vg-/vc-request-with-auth sent, typically shown as "alice@addr verified, introducing myself."
 *     (Bob has verified alice and waits until Alice does the same for him)
 *     1000=vg-member-added/vc-contact-confirm received
 */
#define NC_EVENT_SECUREJOIN_JOINER_PROGRESS       2061


/**
 * The connectivity to the server changed.
 * This means that you should refresh the connectivity view
 * and possibly the connectivtiy HTML; see nc_get_connectivity() and
 * nc_get_connectivity_html() for details.
 *
 * @param data1 0
 * @param data2 0
 */
#define NC_EVENT_CONNECTIVITY_CHANGED             2100


/**
 * The user's avatar changed.
 * You can get the new avatar file with `nc_get_config(context, "selfavatar")`.
 */
#define NC_EVENT_SELFAVATAR_CHANGED               2110


/**
 * A multi-device synced config value changed. Maybe the app needs to refresh smth. For uniformity
 * this is emitted on the source device too. The value isn't reported, otherwise it would be logged
 * which might not be good for privacy. You can get the new value with
 * `nc_get_config(context, data2)`.
 *
 * @param data1 0
 * @param data2 (char*) Configuration key.
 */
#define NC_EVENT_CONFIG_SYNCED                    2111


/**
 * Webxnc status update received.
 * To get the received status update, use nc_get_webxnc_status_updates() with
 * `serial` set to the last known update
 * (in case of special bots, `status_update_serial` from `data2`
 * may help to calculate the last known update for nc_get_webxnc_status_updates();
 * UIs must not peek at this parameter to avoid races in the status replication
 * eg. when events arrive while initial updates are played back).
 *
 * To send status updates, use nc_send_webxnc_status_update().
 *
 * @param data1 (int) msg_id
 * @param data2 (int) status_update_serial - must not be used by UI implementations.
 */
#define NC_EVENT_WEBXNC_STATUS_UPDATE             2120

/**
 * Message deleted which contained a webxnc instance.
 *
 * @param data1 (int) msg_id
 */

#define NC_EVENT_WEBXNC_INSTANCE_DELETED          2121

/**
 * Data received over an ephemeral peer channel.
 *
 * @param data1 (int) msg_id
 * @param data2 (int) + (char*) binary data.
 *     length is returned as integer with nc_event_get_data2_int()
 *     and binary data is returned as nc_event_get_data2_str().
 *     Binary data must be passed to nc_str_unref() afterwards.
 */

#define NC_EVENT_WEBXNC_REALTIME_DATA             2150

/**
 * Advertisement for ephemeral peer channel communication received.
 * This can be used by bots to initiate peer-to-peer communication from their side.
 * @param data1 (int) msg_id
 * @param data2 0
 */

#define NC_EVENT_WEBXNC_REALTIME_ADVERTISEMENT    2151

/**
 * Tells that the Background fetch was completed (or timed out).
 *
 * This event acts as a marker, when you reach this event you can be sure
 * that all events emitted during the background fetch were processed.
 * 
 * This event is only emitted by the account manager
 */

#define NC_EVENT_ACCOUNTS_BACKGROUND_FETCH_DONE   2200

/**
 * Inform that set of chats or the order of the chats in the chatlist has changed.
 *
 * Sometimes this is emitted together with `NC_EVENT_CHATLIST_ITEM_CHANGED`.
 */

#define NC_EVENT_CHATLIST_CHANGED              2300

/**
 * Inform that all or a single chat list item changed and needs to be rerendered
 * If `chat_id` is set to 0, then all currently visible chats need to be rerendered, and all not-visible items need to be cleared from cache if the UI has a cache.
 * 
 * @param data1 (int) chat_id chat id of chatlist item to be rerendered, if chat_id = 0 all (cached & visible) items need to be rerendered
 */

#define NC_EVENT_CHATLIST_ITEM_CHANGED         2301

/**
 * Inform that the list of accounts has changed (an account removed or added or (not yet implemented) the account order changes)
 *
 * This event is only emitted by the account manager.
 */

#define NC_EVENT_ACCOUNTS_CHANGED              2302

/**
 * Inform that an account property that might be shown in the account list changed, namely:
 * - is_configured (see nc_is_configured())
 * - displayname
 * - selfavatar
 * - private_tag
 * 
 * This event is emitted from the account whose property changed.
 */

#define NC_EVENT_ACCOUNTS_ITEM_CHANGED         2303

/**
 * Inform that some events have been skipped due to event channel overflow.
 *
 * @param data1 (int) number of events that have been skipped
 */
#define NC_EVENT_CHANNEL_OVERFLOW              2400



/**
 * Incoming call.
 * UI will usually start ringing,
 * or show a notification if there is already a call in some profile.
 *
 * Together with this event,
 * a message of type #NC_MSG_CALL is added to the corresponding chat;
 * this message is announced and updated by the usual event as #NC_EVENT_MSGS_CHANGED,
 * there is usually no need to take care of this message from any of the CALL events.
 *
 * If user takes action, nc_accept_incoming_call() or nc_end_call() should be called.
 *
 * Otherwise, ringing should end on #NC_EVENT_CALL_ENDED
 * or #NC_EVENT_INCOMING_CALL_ACCEPTED
 *
 * @param data1 (int) msg_id ID of the message referring to the call.
 * @param data2 (char*) place_call_info, text passed to nc_place_outgoing_call()
 * @param data2 (int) 1 if incoming call is a video call, 0 otherwise
 */
#define NC_EVENT_INCOMING_CALL                            2550

/**
 * The callee accepted an incoming call on this or another device using nc_accept_incoming_call().
 * The caller gets the event #NC_EVENT_OUTGOING_CALL_ACCEPTED at the same time.
 *
 * UI usually only takes action in case call UI was opened before, otherwise the event should be ignored.
 *
 * @param data1 (int) msg_id ID of the message referring to the call
 * @param data2 (int) 1 if the call was accepted from this device (process).
 */
 #define NC_EVENT_INCOMING_CALL_ACCEPTED                  2560

/**
 * A call placed using nc_place_outgoing_call() was accepted by the callee using nc_accept_incoming_call().
 *
 * UI usually only takes action in case call UI was opened before, otherwise the event should be ignored.
 *
 * @param data1 (int) msg_id ID of the message referring to the call
 * @param data2 (char*) accept_call_info, text passed to nc_accept_incoming_call()
 */
#define NC_EVENT_OUTGOING_CALL_ACCEPTED                   2570

/**
 * An incoming or outgoing call was ended using nc_end_call() on this or another device, by caller or callee.
 * Moreover, the event is sent when the call was not accepted within 1 minute timeout.
 *
 * UI usually only takes action in case call UI was opened before, otherwise the event should be ignored.
 *
 * @param data1 (int) msg_id ID of the message referring to the call
 */
#define NC_EVENT_CALL_ENDED                               2580

/**
 * Transport relay added/deleted or default has changed.
 * UI should update the list.
 *
 * The event is emitted when the transports are modified on another device
 * using the JSON-RPC calls `add_or_update_transport`, `add_transport_from_qr`, `delete_transport`,
 * `set_transport_unpublished` or `set_config(configured_addr)`.
 */
#define NC_EVENT_TRANSPORTS_MODIFIED           2600


/**
 * @}
 */


#define NC_EVENT_DATA1_IS_STRING(e)  0    // not used anymore 
#define NC_EVENT_DATA2_IS_STRING(e)  ((e)==NC_EVENT_CONFIGURE_PROGRESS || (e)==NC_EVENT_IMEX_FILE_WRITTEN || ((e)>=100 && (e)<=499))


/*
 * Values for nc_get|set_config("show_emails")
 */
#define NC_SHOW_EMAILS_OFF               0
#define NC_SHOW_EMAILS_ACCEPTED_CONTACTS 1
#define NC_SHOW_EMAILS_ALL               2


/*
 * Values for nc_get|set_config("media_quality")
 */
#define NC_MEDIA_QUALITY_BALANCED 0
#define NC_MEDIA_QUALITY_WORSE    1


/**
 * @defgroup NC_PROVIDER_STATUS NC_PROVIDER_STATUS
 *
 * These constants are used as return values for nc_provider_get_status().
 *
 * @addtogroup NC_PROVIDER_STATUS
 * @{
 */

/**
 * Provider works out-of-the-box.
 * This provider status is returned for provider where the login
 * works by just entering the name or the e-mail address.
 *
 * - There is no need for the user to do any special things
 *   (enable IMAP or so) in the provider's web interface or at other places.
 * - There is no need for the user to enter advanced settings;
 *   server, port etc. are known by the core.
 *
 * The status is returned by nc_provider_get_status().
 */
#define         NC_PROVIDER_STATUS_OK           1

/**
 * Provider works, but there are preparations needed.
 *
 * - The user has to do some special things as "Enable IMAP in the web interface",
 *   what exactly, is described in the string returned by nc_provider_get_before_login_hints()
 *   and, typically more detailed, in the page linked by nc_provider_get_overview_page().
 * - There is no need for the user to enter advanced settings;
 *   server, port etc. should be known by the core.
 *
 * The status is returned by nc_provider_get_status().
 */
#define         NC_PROVIDER_STATUS_PREPARATION  2

/**
 * Provider is not working.
 * This provider status is returned for providers
 * that are known to not work with Nexus Chat.
 * The UI should block logging in with this provider.
 *
 * More information about that is typically provided
 * in the string returned by nc_provider_get_before_login_hints()
 * and in the page linked by nc_provider_get_overview_page().
 *
 * The status is returned by nc_provider_get_status().
 */
#define         NC_PROVIDER_STATUS_BROKEN       3

/**
 * @}
 */


/**
 * @defgroup NC_CHAT_VISIBILITY NC_CHAT_VISIBILITY
 *
 * These constants describe the visibility of a chat.
 * The chat visibility can be get using nc_chat_get_visibility()
 * and set using nc_set_chat_visibility().
 *
 * @addtogroup NC_CHAT_VISIBILITY
 * @{
 */

/**
 * Chats with normal visibility are not archived and are shown below all pinned chats.
 * Archived chats, that receive new messages automatically become normal chats.
 */
#define         NC_CHAT_VISIBILITY_NORMAL      0

/**
 * Archived chats are not included in the default chatlist returned by nc_get_chatlist().
 * Instead, if there are _any_ archived chats, the pseudo-chat
 * with the chat ID NC_CHAT_ID_ARCHIVED_LINK will be added at the end of the chatlist.
 *
 * The UI typically shows a little icon or chats beside archived chats in the chatlist,
 * this is needed as e.g. the search will also return archived chats.
 *
 * If archived chats receive new messages, they become normal chats again.
 *
 * To get a list of archived chats, use nc_get_chatlist() with the flag NC_GCL_ARCHIVED_ONLY.
 */
#define         NC_CHAT_VISIBILITY_ARCHIVED    1

/**
 * Pinned chats are included in the default chatlist. moreover,
 * they are always the first items, whether they have fresh messages or not.
 */
#define         NC_CHAT_VISIBILITY_PINNED      2

/**
 * @}
 */


/**
  * @defgroup NC_DOWNLOAD NC_DOWNLOAD
  *
  * These constants describe the download state of a message.
  * The download state can be retrieved using nc_msg_get_download_state()
  * and usually changes after calling nc_download_full_msg().
  *
  * @addtogroup NC_DOWNLOAD
  * @{
  */

/**
 * Download not needed, see nc_msg_get_download_state() for details.
 */
#define NC_DOWNLOAD_DONE           0

/**
 * Download available, see nc_msg_get_download_state() for details.
 */
#define NC_DOWNLOAD_AVAILABLE      10

/**
 * Download failed, see nc_msg_get_download_state() for details.
 */
#define NC_DOWNLOAD_FAILURE        20

/**
 * Download not needed, see nc_msg_get_download_state() for details.
 */
#define NC_DOWNLOAD_UNDECIPHERABLE 30

/**
 * Download in progress, see nc_msg_get_download_state() for details.
 */
#define NC_DOWNLOAD_IN_PROGRESS    1000



/**
 * @}
 */


/**
 * @defgroup NC_STR NC_STR
 *
 * These constants are used to define strings using nc_set_stock_translation().
 * This allows localisation of the texts used by the core,
 * you have to call nc_set_stock_translation()
 * for every @ref NC_STR string you want to translate.
 *
 * Some strings contain some placeholders as `%1$s` or `%2$s` -
 * these will be replaced by some content defined in the @ref NC_STR description below.
 * As a synonym for `%1$s` you can also use `%1$d` or `%1$@`; same for `%2$s`.
 *
 * If you do not call nc_set_stock_translation() for a concrete @ref NC_STR constant,
 * a default string will be used.
 *
 * @addtogroup NC_STR
 * @{
 */

/// "No messages."
///
/// Used in summaries.
#define NC_STR_NOMESSAGES                 1

/// "Me"
///
/// Used as the sender name for oneself.
#define NC_STR_SELF                       2

/// "Draft"
///
/// Used in summaries.
#define NC_STR_DRAFT                      3

/// "Voice message"
///
/// Used in summaries.
#define NC_STR_VOICEMESSAGE               7

/// "Image"
///
/// Used in summaries.
#define NC_STR_IMAGE                      9

/// "Video"
///
/// Used in summaries.
#define NC_STR_VIDEO                      10

/// "Audio"
///
/// Used in summaries.
#define NC_STR_AUDIO                      11

/// "File"
///
/// Used in summaries.
#define NC_STR_FILE                       12

/// "GIF"
///
/// Used in summaries.
#define NC_STR_GIF                        23

/// "End-to-end encryption available."
///
/// @deprecated 2026-01-23
#define NC_STR_E2E_AVAILABLE              25

/// "No encryption."
///
/// Used to build the string returned by nc_get_contact_encrinfo().
#define NC_STR_ENCR_NONE                  28

/// "Fingerprints"
///
/// Used to build the string returned by nc_get_contact_encrinfo().
#define NC_STR_FINGERPRINTS               30

/// "%1$s verified"
///
/// Used in status messages.
/// - %1$s will be replaced by the name of the verified contact
#define NC_STR_CONTACT_VERIFIED           35

/// "Archived chats"
///
/// Used as the name for the corresponding chatlist entry.
#define NC_STR_ARCHIVENCHATS              40

/// "Cannot login as %1$s."
///
/// Used in error strings.
/// - %1$s will be replaced by the failing login name
#define NC_STR_CANNOT_LOGIN               60

/// "Location streaming enabled."
///
/// Used in status messages.
#define NC_STR_MSGLOCATIONENABLED         64

/// "Location streaming disabled."
///
/// Used in status messages.
#define NC_STR_MSGLOCATIONDISABLED        65

/// "Location"
///
/// Used in summaries.
#define NC_STR_LOCATION                   66

/// "Sticker"
///
/// Used in summaries.
#define NC_STR_STICKER                    67

/// "Device messages"
///
/// Used as the name for the corresponding chat.
#define NC_STR_DEVICE_MESSAGES            68

/// "Saved messages"
///
/// Used as the name for the corresponding chat.
#define NC_STR_SAVED_MESSAGES             69

/// "Messages in this chat are generated locally by your Nexus Chat app."
///
/// Used as message text for the message added to a newly created device chat.
#define NC_STR_DEVICE_MESSAGES_HINT       70

/// "Welcome to Nexus Chat! Nexus Chat looks and feels like other popular messenger apps ..."
///
/// Used as message text for the message added to the device chat after successful login.
#define NC_STR_WELCOME_MESSAGE            71

/// "Message from %1$s"
///
/// Used in subjects of outgoing messages in one-to-one chats.
/// - %1$s will be replaced by the name of the sender,
///   this is the nc_set_config()-option `displayname` or `addr`
#define NC_STR_SUBJECT_FOR_NEW_CONTACT    73

/// "Failed to send message to %1$s."
///
/// Unused. Was used in group chat status messages.
/// - %1$s will be replaced by the name of the contact the message cannot be sent to
#define NC_STR_FAILED_SENDING_TO          74

/// "Error: %1$s"
///
/// Used in error strings.
/// - %1$s will be replaced by the concrete error
#define NC_STR_CONFIGURATION_FAILED       84

/// "Date or time of your device seem to be inaccurate (%1$s). Adjust your clock to ensure your messages are received correctly."
///
/// Used as device message if a wrong date or time was detected.
/// - %1$s will be replaced by a date/time string as YY-mm-dd HH:MM:SS
#define NC_STR_BAD_TIME_MSG_BODY          85

/// "Your Nexus Chat version might be outdated, check https://get.nexus.chat for updates."
///
/// Used as device message if the used version is probably outdated.
#define NC_STR_UPDATE_REMINDER_MSG_BODY   86

/// "No network."
///
/// Used in error strings.
#define NC_STR_ERROR_NO_NETWORK           87

/// "Reply"
///
/// Used in summaries.
/// Note: the string has to be a noun, not a verb (not: "to reply").
#define NC_STR_REPLY_NOUN                 90

/// "You deleted the 'Saved messages' chat..."
///
/// Used as device message text.
#define NC_STR_SELF_DELETED_MSG_BODY      91

/// "Forwarded"
///
/// Used in message summary text for notifications and chatlist.
#define NC_STR_FORWARDED                  97

/// "Quota exceeding, already %1$s%% used."
///
/// Used as device message text.
///
/// `%1$s` will be replaced by the percentage used
#define NC_STR_QUOTA_EXCEEDING_MSG_BODY   98

/// "Multi Device Synchronization"
///
/// Used in subjects of outgoing sync messages.
#define NC_STR_SYNC_MSG_SUBJECT           101

/// "This message is used to synchronize data between your devices."
///
///
/// Used as message text of outgoing sync messages.
/// The text is visible in non-nc-muas or in outdated Nexus Chat versions,
/// the default text therefore adds the following hint:
/// "If you see this message in Nexus Chat,
/// please update your Nexus Chat apps on all devices."
#define NC_STR_SYNC_MSG_BODY              102

/// "Incoming Messages"
///
/// Used as a headline in the connectivity view.
#define NC_STR_INCOMING_MESSAGES          103

/// "Outgoing Messages"
///
/// Used as a headline in the connectivity view.
#define NC_STR_OUTGOING_MESSAGES          104

/// "Connected"
///
/// Used as status in the connectivity view.
#define NC_STR_CONNECTED                  107

/// "Connecting…"
///
/// Used as status in the connectivity view.
#define NC_STR_CONNTECTING                108

/// "Updating…"
///
/// Used as status in the connectivity view.
#define NC_STR_UPDATING                   109

/// "Sending…"
///
/// Used as status in the connectivity view.
#define NC_STR_SENDING                    110

/// "Your last message was sent successfully."
///
/// Used as status in the connectivity view.
#define NC_STR_LAST_MSG_SENT_SUCCESSFULLY 111

/// "Error: %1$s"
///
/// Used as status in the connectivity view.
///
/// `%1$s` will be replaced by a possibly more detailed, typically english, error description.
#define NC_STR_ERROR                      112

/// "Not supported by your provider."
///
/// Used in the connectivity view.
#define NC_STR_NOT_SUPPORTED_BY_PROVIDER  113

/// "Messages"
///
/// Used as a subtitle in quota context; can be plural always.
#define NC_STR_MESSAGES                   114

/// "Broancast List"
///
/// Used as the default name for broancast lists; a number may be added.
#define NC_STR_BROANCAST_LIST             115

/// "%1$s of %2$s used"
///
/// Used for describing resource usage, resulting string will be e.g. "1.2 GiB of 3 GiB used".
#define NC_STR_PART_OF_TOTAL_USED         116

/// "%1$s invited you to join this group. Waiting for the device of %2$s to reply…"
///
/// Added as an info-message directly after scanning a QR code for joining a group.
/// May be followed by the info-messages
/// #NC_STR_SECURE_JOIN_REPLIES, #NC_STR_CONTACT_VERIFIED and #NC_STR_MSGADDMEMBER.
///
/// `%1$s` and `%2$s` will be replaced by name of the inviter.
#define NC_STR_SECURE_JOIN_STARTED        117

/// "%1$s replied, waiting for being added to the group…"
///
/// Info-message on scanning a QR code for joining a group.
/// Added after #NC_STR_SECURE_JOIN_STARTED.
/// If the handshake allows to skip a step and go for #NC_STR_CONTACT_VERIFIED directly,
/// this info-message is skipped.
///
/// `%1$s` will be replaced by the name of the inviter.
#define NC_STR_SECURE_JOIN_REPLIES        118

/// "Scan to chat with %1$s"
///
/// Subtitle for verification qrcode svg image generated by the core.
///
/// `%1$s` will be replaced by name of the inviter.
#define NC_STR_SETUP_CONTACT_QR_DESC      119

/// "Scan to join %1$s"
///
/// Subtitle for group join qrcode svg image generated by the core.
///
/// `%1$s` will be replaced with the group name.
#define NC_STR_SECURE_JOIN_GROUP_QR_DESC  120

/// "Not connected"
///
/// Used as status in the connectivity view.
#define NC_STR_NOT_CONNECTED              121

/// "You changed group name from \"%1$s\" to \"%2$s\"."
///
/// `%1$s` will be replaced by the old group name.
/// `%2$s` will be replaced by the new group name.
#define NC_STR_GROUP_NAME_CHANGED_BY_YOU 124

/// "Group name changed from \"%1$s\" to \"%2$s\" by %3$s."
///
/// `%1$s` will be replaced by the old group name.
/// `%2$s` will be replaced by the new group name.
/// `%3$s` will be replaced by name of the contact who did the action.
#define NC_STR_GROUP_NAME_CHANGED_BY_OTHER 125

/// "You changed the group image."
#define NC_STR_GROUP_IMAGE_CHANGED_BY_YOU 126

/// "Group image changed by %1$s."
///
/// `%1$s` will be replaced by name of the contact who did the action.
#define NC_STR_GROUP_IMAGE_CHANGED_BY_OTHER 127

/// "You added member %1$s."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the added member's name.
#define NC_STR_ADD_MEMBER_BY_YOU 128

/// "Member %1$s added by %2$s."
///
/// `%1$s` will be replaced by name of the contact added to the group.
/// `%2$s` will be replaced by name of the contact who did the action.
///
/// Used in status messages.
#define NC_STR_ADD_MEMBER_BY_OTHER 129

/// "You removed member %1$s."
///
/// `%1$s` will be replaced by name of the contact removed from the group.
///
/// Used in status messages.
#define NC_STR_REMOVE_MEMBER_BY_YOU 130

/// "Member %1$s removed by %2$s."
///
/// `%1$s` will be replaced by name of the contact removed from the group.
/// `%2$s` will be replaced by name of the contact who did the action.
///
/// Used in status messages.
#define NC_STR_REMOVE_MEMBER_BY_OTHER 131

/// "You left the group."
///
/// Used in status messages.
#define NC_STR_GROUP_LEFT_BY_YOU 132

/// "Group left by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_GROUP_LEFT_BY_OTHER 133

/// "You deleted the group image."
///
/// Used in status messages.
#define NC_STR_GROUP_IMAGE_DELETED_BY_YOU 134

/// "Group image deleted by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_GROUP_IMAGE_DELETED_BY_OTHER 135

/// "You enabled location streaming."
///
/// Used in status messages.
#define NC_STR_LOCATION_ENABLED_BY_YOU 136

/// "Location streaming enabled by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_LOCATION_ENABLED_BY_OTHER 137

/// "You disabled message deletion timer."
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_DISABLED_BY_YOU 138

/// "Message deletion timer is disabled by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_DISABLED_BY_OTHER 139

/// "You set message deletion timer to %1$s s."
///
/// `%1$s` will be replaced by the number of seconds (always >1) the timer is set to.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_SECONDS_BY_YOU 140

/// "Message deletion timer is set to %1$s s by %2$s."
///
/// `%1$s` will be replaced by the number of seconds (always >1) the timer is set to.
/// `%2$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_SECONDS_BY_OTHER 141

/// "You set message deletion timer to 1 hour."
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_HOUR_BY_YOU 144

/// "Message deletion timer is set to 1 hour by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_HOUR_BY_OTHER 145

/// "You set message deletion timer to 1 day."
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_DAY_BY_YOU 146

/// "Message deletion timer is set to 1 day by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_DAY_BY_OTHER 147

/// "You set message deletion timer to 1 week."
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_WEEK_BY_YOU 148

/// "Message deletion timer is set to 1 week by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_WEEK_BY_OTHER 149

/// "You set message deletion timer to %1$s minutes."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of minutes (always >1) the timer is set to.
#define NC_STR_EPHEMERAL_TIMER_MINUTES_BY_YOU 150

/// "Message deletion timer is set to %1$s minutes by %2$s."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of minutes (always >1) the timer is set to.
/// `%2$s` will be replaced by name of the contact.
#define NC_STR_EPHEMERAL_TIMER_MINUTES_BY_OTHER 151

/// "You set message deletion timer to %1$s hours."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of hours (always >1) the timer is set to.
#define NC_STR_EPHEMERAL_TIMER_HOURS_BY_YOU 152

/// "Message deletion timer is set to %1$s hours by %2$s."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of hours (always >1) the timer is set to.
/// `%2$s` will be replaced by name of the contact.
#define NC_STR_EPHEMERAL_TIMER_HOURS_BY_OTHER 153

/// "You set message deletion timer to %1$s days."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of days (always >1) the timer is set to.
#define NC_STR_EPHEMERAL_TIMER_DAYS_BY_YOU 154

/// "Message deletion timer is set to %1$s days by %2$s."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of days (always >1) the timer is set to.
/// `%2$s` will be replaced by name of the contact.
#define NC_STR_EPHEMERAL_TIMER_DAYS_BY_OTHER 155

/// "You set message deletion timer to %1$s weeks."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of weeks (always >1) the timer is set to.
#define NC_STR_EPHEMERAL_TIMER_WEEKS_BY_YOU 156

/// "Message deletion timer is set to %1$s weeks by %2$s."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the number of weeks (always >1) the timer is set to.
/// `%2$s` will be replaced by name of the contact.
#define NC_STR_EPHEMERAL_TIMER_WEEKS_BY_OTHER 157

/// "You set message deletion timer to 1 year."
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_YEAR_BY_YOU 158

/// "Message deletion timer is set to 1 year by %1$s."
///
/// `%1$s` will be replaced by name of the contact.
///
/// Used in status messages.
#define NC_STR_EPHEMERAL_TIMER_1_YEAR_BY_OTHER 159

/// "Scan to set up second device for %1$s"
///
/// `%1$s` will be replaced by name of the account.
#define NC_STR_BACKUP_TRANSFER_QR 162

/// "Account transferred to your second device."
///
/// Used as a device message after a successful backup transfer.
#define NC_STR_BACKUP_TRANSFER_MSG_BODY 163

/// "Messages are end-to-end encrypted."
///
/// Used in info-messages, UI may add smth. as "Tap to learn more."
#define NC_STR_CHAT_PROTECTION_ENABLED 170

/// "Others will only see this group after you sent a first message."
///
/// Used as the first info messages in newly created groups.
#define NC_STR_NEW_GROUP_SEND_FIRST_MESSAGE 172

/// "Member %1$s added."
///
/// Used as info messages.
///
/// `%1$s` will be replaced by the added member's name.
#define NC_STR_MESSAGE_ADD_MEMBER 173

/// "Your email provider %1$s requires end-to-end encryption which is not setup yet."
///
/// Used as info messages when a message cannot be sent because it cannot be encrypted.
///
/// `%1$s` will be replaced by the provider's domain.
#define NC_STR_INVALID_UNENCRYPTED_MAIL 174

/// "You reacted %1$s to '%2$s'"
///
/// `%1$s` will be replaced by the reaction, usually an emoji
/// `%2$s` will be replaced by the summary of the message the reaction refers to
///
/// Used in summaries.
#define NC_STR_YOU_REACTED 176

/// "%1$s reacted %2$s to '%3$s'"
///
/// `%1$s` will be replaced by the name the contact who reacted
/// `%2$s` will be replaced by the reaction, usually an emoji
/// `%3$s` will be replaced by the summary of the message the reaction refers to
///
/// Used in summaries.
#define NC_STR_REACTED_BY 177

/// "Member %1$s removed."
///
/// `%1$s` will be replaced by name of the removed contact.
#define NC_STR_REMOVE_MEMBER 178

/// "Establishing connection, please wait…"
///
/// Used as info message.
#define NC_STR_SECUREJOIN_WAIT 190

/// "❤️ Seems you're enjoying Nexus Chat!"… (donation request device message)
#define NC_STR_DONATION_REQUEST 193

/// "Declined call"
#define NC_STR_DECLINED_CALL 196

/// "Canceled call"
#define NC_STR_CANCELED_CALL 197

/// "Missed call"
#define NC_STR_MISSED_CALL 198

/// "You left the channel."
///
/// Used in status messages.
#define NC_STR_CHANNEL_LEFT_BY_YOU 200

/// "Scan to join channel %1$s"
///
/// Subtitle for channel join qrcode svg image generated by the core.
///
/// `%1$s` will be replaced with the channel name.
#define NC_STR_SECURE_JOIN_CHANNEL_QR_DESC 201

/// "You joined the channel."
#define NC_STR_MSG_YOU_JOINED_CHANNEL 202

/// "%1$s invited you to join this channel. Waiting for the device of %2$s to reply…"
///
/// Added as an info-message directly after scanning a QR code for joining a broancast channel.
///
/// `%1$s` and `%2$s` will both be replaced by the name of the inviter.
#define NC_STR_SECURE_JOIN_CHANNEL_STARTED 203

/// "Channel name changed from %1$s to %2$s."
///
/// Used in status messages.
///
/// `%1$s` will be replaced by the old channel name.
/// `%2$s` will be replaced by the new channel name.
#define NC_STR_CHANNEL_NAME_CHANGED 204

/// "Channel image changed."
///
/// Used in status messages.
#define NC_STR_CHANNEL_IMAGE_CHANGED 205

/// "The attachment contains anonymous usage statistics, which help us improve Nexus Chat. Thank you!"
///
/// Used as the message body for statistics sent out.
#define NC_STR_STATS_MSG_BODY 210

/// "Proxy Enabled"
///
/// Title for proxy section in connectivity view.
#define NC_STR_PROXY_ENABLED  220

/// "You are using a proxy. If you're having trouble connecting, try a different proxy."
///
/// Description in connectivity view when proxy is enabled.
#define NC_STR_PROXY_ENABLED_DESCRIPTION  221

/// "Messages in this chat use classic email and are not encrypted."
///
/// Used as the first info messages in newly created classic email threads.
#define NC_STR_CHAT_UNENCRYPTED_EXPLANATON 230

/// "Outgoing audio call"
#define NC_STR_OUTGOING_AUDIO_CALL 232

/// "Outgoing video call"
#define NC_STR_OUTGOING_VIDEO_CALL 233

/// "Incoming audio call"
#define NC_STR_INCOMING_AUDIO_CALL 234

/// "Incoming video call"
#define NC_STR_INCOMING_VIDEO_CALL 235

/// "You changed the chat description."
#define NC_STR_GROUP_DESCRIPTION_CHANGED_BY_YOU 240

/// "Chat description changed by %1$s."
#define NC_STR_GROUP_DESCRIPTION_CHANGED_BY_OTHER 241

/// "Messages are end-to-end encrypted."
///
/// Used when creating text for the "Encryption Info" dialogs.
#define NC_STR_MESSAGES_ARE_E2EE 242

/**
 * @}
 */


#ifdef PY_CFFI_INC
/* Helper utility to locate the header file when building python bindings. */
char* _nc_header_file_location(void) {
    return __FILE__;
}
#endif


#ifdef __cplusplus
}
#endif
#endif // __DELTACHAT_H__
