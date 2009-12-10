/* Copyright (C) 2009 - Virtualsquare Team
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/

#ifndef __VDE3_PRIV_COMPONENT_H__
#define __VDE3_PRIV_COMPONENT_H__

// le operazioni che il contesto invoca sul componente
struct component_ops {
  char* .new = NULL, // called when a context creates a new component
  char* .init = NULL,// called when a context init a new component
  char* .fini = NULL,// called when a context closes a component
  char* .delete = NULL,// called when a context destroys a component 
  char* .get_configuration = NULL, // called to get a serializable config
  char* .set_configuration = NULL, // called to set a serializable config
  char* .get_policy = NULL, // called to get a serializable policy
  char* .set_policy = NULL, // called to set a serializable policy
};

// per adesso usiamo le union, poi vediamo come mettere qualcosa che permetta
// di avere sotto-sottoclassi in un modo piu` pulito (adesso non si riesce a
// fare una sottoclasse di transport.. ma nemmeno ci serve per ora)
struct vde_component {
  struct component_ops *cops;
  union ops {
    struct transport_ops *transport;
    struct engine_ops *engine;
    struct connection_manager_ops *connection_manager;
  },
  char *kind, // XXX(godog): enum ?
  char *family,
  char *name,
  int refcnt,
  struct vde_command commands[];
  struct vde_signal signals[];
  void *priv,
};

/** 
* @brief Increase reference counter
* 
* @param component The component 
* @param count The pointer where to store reference counter value (might be
* NULL)
* 
* @return zero on success, otherwise an error code
*/
int vde_component_get(vde_component *component, int *count);

/** 
* @brief Decrease reference counter
* 
* @param component The component
* @param count The pointer where to store reference counter value (might be
* NULL)
* 
* @return zero on success, otherwise an error code
*/
/* XXX(shammash):
 *  - define if the component must be fini()+free() automatically when the
 *    refcounter reaches zero
 */
int vde_component_put(vde_component *component, int *count);

/** 
* @brief vde_component utility to add a command
* 
* @param component The component to add the command to
* @param command The command to add
* 
* @return zero on success, otherwise an error code
*/
int vde_component_command_add(vde_component *component,
                               vde_command *command);

/** 
* @brief vde_component utility to remove a command
* 
* @param component The component to remove the command from
* @param command The command to remove
* 
* @return zero on success, otherwise an error code
*/
int vde_component_command_del(vde_component *component,
                               vde_command *command);

/** 
* @brief Lookup for a command in a component
* 
* @param component The component to look into
* @param name The name of the command
* 
* @return a vde command, NULL if not found
*/
vde_command *vde_component_command_get(vde_component *component,
                                        const char *name);

/** 
* @brief List all commands of a component
* 
* @param component The component 
* 
* @return A null terminated array of commands
*/
vde_command **vde_component_commands_list(vde_component *component);

/** 
* @brief vde_component utility to add a signal
* 
* @param component The component to add the signal to
* @param signal The signal to add
* 
* @return zero on success, otherwise an error code
*/
int vde_component_signal_add(vde_component *component,
                              vde_signal *signal);

/** 
* @brief vde_component utility to remove a signal
* 
* @param component The component to remove the signal from
* @param signal The signal to remove
* 
* @return zero on success, otherwise an error code
*/
int vde_component_signal_del(vde_component *component,
                              vde_signal *signal);

/** 
* @brief Lookup for a signal in a component
* 
* @param component The component to look into
* @param name The name of the signal
* 
* @return a vde signal, NULL if not found
*/
vde_signal *vde_component_signal_get(vde_component *component,
                                       const char *name);

/** 
* @brief List all signals of a component
* 
* @param component The component 
* 
* @return A null terminated array of signals
*/
vde_signals **vde_component_signals_list(vde_component *component);

/** 
* @brief Signature of a signal callback
* 
* @param component The component raising the signal
* @param signal The signal name
* @param infos Serialized signal parameters, if NULL the signal is being
*              destroyed
* @param data Callback private data
*/
void vde_component_signal_callback(vde_component *component,
                                    const char *signal, json_object *infos,
                                    void *data);

/** 
* @brief Attach a callback to a signal
* 
* @param component The component to receive signals from
* @param signal The signal name
* @param callback The callback function
* @param data Callback private data
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_attach(vde_component *component, const char *signal,
                                  vde_component_signal_callback (*callback),
                                  void *data);

/** 
* @brief Detach a callback from a signal
* 
* @param component The component to stop receiving signals from
* @param signal The signal name
* @param callback The callback function to detach
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_detach(vde_component *component, const char *signal,
                                 vde_component_signal_callback (*callback));

#endif /* __VDE3_PRIV_COMPONENT_H__ */
