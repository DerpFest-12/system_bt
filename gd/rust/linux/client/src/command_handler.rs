use std::collections::HashMap;
use std::sync::{Arc, Mutex};

use num_traits::cast::FromPrimitive;

use crate::ClientContext;
use crate::{console_red, console_yellow, print_error, print_info};
use btstack::bluetooth::{BluetoothDevice, BluetoothTransport, IBluetooth};
use manager_service::iface_bluetooth_manager::IBluetoothManager;

const INDENT_CHAR: &str = " ";
const BAR1_CHAR: &str = "=";
const BAR2_CHAR: &str = "-";
const MAX_MENU_CHAR_WIDTH: usize = 72;

type CommandFunction = fn(&mut CommandHandler, &Vec<String>);

fn _noop(_handler: &mut CommandHandler, _args: &Vec<String>) {
    // Used so we can add options with no direct function
    // e.g. help and quit
}

pub struct CommandOption {
    description: String,
    function_pointer: CommandFunction,
}

/// Handles string command entered from command line.
pub(crate) struct CommandHandler {
    context: Arc<Mutex<ClientContext>>,
    command_options: HashMap<String, CommandOption>,
}

fn enforce_arg_len<F>(args: &Vec<String>, min_len: usize, msg: &str, mut action: F)
where
    F: FnMut(),
{
    if args.len() < min_len {
        println!("Usage: {}", msg);
    } else {
        action();
    }
}

fn wrap_help_text(text: &str, max: usize, indent: usize) -> String {
    let remaining_count = std::cmp::max(
        // real_max
        std::cmp::max(max, text.chars().count())
        // take away char count
         - text.chars().count()
        // take away real_indent
         - (
             if std::cmp::max(max, text.chars().count())- text.chars().count() > indent {
                 indent
             } else {
                 0
             }),
        0,
    );

    format!("|{}{}{}|", INDENT_CHAR.repeat(indent), text, INDENT_CHAR.repeat(remaining_count))
}

// This should be called during the constructor in order to populate the command option map
fn build_commands() -> HashMap<String, CommandOption> {
    let mut command_options = HashMap::<String, CommandOption>::new();
    command_options.insert(
        String::from("adapter"),
        CommandOption {
            description: String::from(
                "Enable/Disable/Show default bluetooth adapter. (e.g. adapter enable)",
            ),
            function_pointer: CommandHandler::cmd_adapter,
        },
    );
    command_options.insert(
        String::from("bond"),
        CommandOption {
            description: String::from("Creates a bond with a device."),
            function_pointer: CommandHandler::cmd_bond,
        },
    );
    command_options.insert(
        String::from("discovery"),
        CommandOption {
            description: String::from("Start and stop device discovery. (e.g. discovery start)"),
            function_pointer: CommandHandler::cmd_discovery,
        },
    );
    command_options.insert(
        String::from("get-address"),
        CommandOption {
            description: String::from("Gets the local device address."),
            function_pointer: CommandHandler::cmd_get_address,
        },
    );
    command_options.insert(
        String::from("help"),
        CommandOption {
            description: String::from("Shows this menu."),
            function_pointer: CommandHandler::cmd_help,
        },
    );
    command_options.insert(
        String::from("list-devices"),
        CommandOption {
            description: String::from(
                "List known remote devices from most recent discovery session.",
            ),
            function_pointer: CommandHandler::cmd_list_devices,
        },
    );
    command_options.insert(
        String::from("quit"),
        CommandOption {
            description: String::from("Quit out of the interactive shell."),
            function_pointer: _noop,
        },
    );
    command_options
}

impl CommandHandler {
    /// Creates a new CommandHandler.
    pub fn new(context: Arc<Mutex<ClientContext>>) -> CommandHandler {
        CommandHandler { context, command_options: build_commands() }
    }

    /// Entry point for command and arguments
    pub fn process_cmd_line(&mut self, command: &String, args: &Vec<String>) {
        // Ignore empty line
        match &command[0..] {
            "" => {}
            _ => match self.command_options.get(command) {
                Some(cmd) => (cmd.function_pointer)(self, &args),
                None => {
                    println!("'{}' is an invalid command!", command);
                    self.cmd_help(&args);
                }
            },
        };
    }

    //  Common message for when the adapter isn't ready
    fn adapter_not_ready(&self) {
        let adapter_idx = self.context.lock().unwrap().default_adapter;
        print_error!(
            "Default adapter {} is not enabled. Enable the adapter before using this command.",
            adapter_idx
        );
    }

    fn cmd_help(&mut self, args: &Vec<String>) {
        if args.len() > 0 {
            match self.command_options.get(&args[0]) {
                Some(cmd) => {
                    println!(
                        "\n{}{}\n{}{}\n",
                        INDENT_CHAR.repeat(4),
                        args[0],
                        INDENT_CHAR.repeat(8),
                        cmd.description
                    );
                }
                None => {
                    println!("'{}' is an invalid command!", args[0]);
                    self.cmd_help(&vec![]);
                }
            }
        } else {
            // Build equals bar and Shave off sides
            let equal_bar = format!(" {} ", BAR1_CHAR.repeat(MAX_MENU_CHAR_WIDTH));

            // Build empty bar and Shave off sides
            let empty_bar = format!("|{}|", INDENT_CHAR.repeat(MAX_MENU_CHAR_WIDTH));

            // Header
            println!(
                "\n{}\n{}\n{}\n{}",
                equal_bar,
                wrap_help_text("Help Menu", MAX_MENU_CHAR_WIDTH, 2),
                // Minus bar
                format!("+{}+", BAR2_CHAR.repeat(MAX_MENU_CHAR_WIDTH)),
                empty_bar
            );

            // Print commands
            for (key, val) in self.command_options.iter() {
                println!(
                    "{}\n{}\n{}",
                    wrap_help_text(&key, MAX_MENU_CHAR_WIDTH, 4),
                    wrap_help_text(&val.description, MAX_MENU_CHAR_WIDTH, 8),
                    empty_bar
                );
            }

            // Footer
            println!("{}\n{}", empty_bar, equal_bar);
        }
    }

    fn cmd_adapter(&mut self, args: &Vec<String>) {
        let default_adapter = self.context.lock().unwrap().default_adapter;
        enforce_arg_len(args, 1, "adapter <enable|disable|show>", || match &args[0][0..] {
            "enable" => {
                self.context.lock().unwrap().manager_dbus.start(default_adapter);
            }
            "disable" => {
                self.context.lock().unwrap().manager_dbus.stop(default_adapter);
            }
            "show" => {
                let enabled = self.context.lock().unwrap().enabled;
                let address = match self.context.lock().unwrap().adapter_address.as_ref() {
                    Some(x) => x.clone(),
                    None => String::from(""),
                };
                print_info!("State: {}", if enabled { "enabled" } else { "disabled" });
                print_info!("Address: {}", address);
            }
            _ => {
                println!("Invalid argument '{}'", args[0]);
            }
        });
    }

    fn cmd_get_address(&mut self, _args: &Vec<String>) {
        if !self.context.lock().unwrap().adapter_ready {
            self.adapter_not_ready();
            return;
        }

        let address = self.context.lock().unwrap().adapter_dbus.as_ref().unwrap().get_address();
        print_info!("Local address = {}", &address);
        // Cache address for adapter show
        self.context.lock().unwrap().adapter_address = Some(address);
    }

    fn cmd_discovery(&mut self, args: &Vec<String>) {
        if !self.context.lock().unwrap().adapter_ready {
            self.adapter_not_ready();
            return;
        }

        enforce_arg_len(args, 1, "discovery <start|stop>", || match &args[0][0..] {
            "start" => {
                self.context.lock().unwrap().adapter_dbus.as_ref().unwrap().start_discovery();
            }
            "stop" => {
                self.context.lock().unwrap().adapter_dbus.as_ref().unwrap().cancel_discovery();
            }
            _ => {
                println!("Invalid argument '{}'", args[0]);
            }
        });
    }

    fn cmd_bond(&mut self, args: &Vec<String>) {
        if !self.context.lock().unwrap().adapter_ready {
            self.adapter_not_ready();
            return;
        }

        enforce_arg_len(args, 1, "bond <address>", || {
            let device = BluetoothDevice {
                address: String::from(&args[0]),
                name: String::from("Classic Device"),
            };

            self.context
                .lock()
                .unwrap()
                .adapter_dbus
                .as_ref()
                .unwrap()
                .create_bond(device, BluetoothTransport::from_i32(0).unwrap());
        });
    }

    /// Get the list of currently supported commands
    pub fn get_command_list(&self) -> Vec<String> {
        self.command_options.keys().map(|key| String::from(key)).collect::<Vec<String>>()
    }

    fn cmd_list_devices(&mut self, _args: &Vec<String>) {
        print_info!("Devices found in most recent discovery session:");
        for (key, val) in self.context.lock().unwrap().found_devices.iter() {
            print_info!("[{:18}] {}", key, val.name);
        }
    }
}

#[cfg(test)]
mod tests {

    use super::*;

    #[test]
    fn test_wrap_help_text() {
        let text = "hello";
        let text_len = text.chars().count();
        // ensure no overflow
        assert_eq!(format!("|{}|", text), wrap_help_text(text, 4, 0));
        assert_eq!(format!("|{}|", text), wrap_help_text(text, 5, 0));
        assert_eq!(format!("|{}{}|", text, " "), wrap_help_text(text, 6, 0));
        assert_eq!(format!("|{}{}|", text, " ".repeat(2)), wrap_help_text(text, 7, 0));
        assert_eq!(
            format!("|{}{}|", text, " ".repeat(100 - text_len)),
            wrap_help_text(text, 100, 0)
        );
        assert_eq!(format!("|{}{}|", " ", text), wrap_help_text(text, 4, 1));
        assert_eq!(format!("|{}{}|", " ".repeat(2), text), wrap_help_text(text, 5, 2));
        assert_eq!(format!("|{}{}{}|", " ".repeat(3), text, " "), wrap_help_text(text, 6, 3));
        assert_eq!(
            format!("|{}{}{}|", " ".repeat(4), text, " ".repeat(7 - text_len)),
            wrap_help_text(text, 7, 4)
        );
        assert_eq!(format!("|{}{}|", " ".repeat(9), text), wrap_help_text(text, 4, 9));
        assert_eq!(format!("|{}{}|", " ".repeat(10), text), wrap_help_text(text, 3, 10));
        assert_eq!(format!("|{}{}|", " ".repeat(11), text), wrap_help_text(text, 2, 11));
        assert_eq!(format!("|{}{}|", " ".repeat(12), text), wrap_help_text(text, 1, 12));
        assert_eq!("||", wrap_help_text("", 0, 0));
        assert_eq!("| |", wrap_help_text("", 1, 0));
        assert_eq!("|  |", wrap_help_text("", 1, 1));
        assert_eq!("| |", wrap_help_text("", 0, 1));
    }

    #[test]
    fn test_enforce_arg_len() {
        // With min arg set and min arg supplied
        let args: &Vec<String> = &vec![String::from("arg")];
        let mut i: usize = 0;
        enforce_arg_len(args, 1, "help text", || {
            i = 1;
        });
        assert_eq!(1, i);

        // With no min arg set and with arg supplied
        i = 0;
        enforce_arg_len(args, 0, "help text", || {
            i = 1;
        });
        assert_eq!(1, i);

        // With min arg set and no min arg supplied
        let args: &Vec<String> = &vec![];
        i = 0;
        enforce_arg_len(args, 1, "help text", || {
            i = 1;
        });
        assert_eq!(0, i);

        // With no min arg set and no arg supplied
        i = 0;
        enforce_arg_len(args, 0, "help text", || {
            i = 1;
        });
        assert_eq!(1, i);
    }
}
