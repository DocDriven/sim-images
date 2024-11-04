use clap::Parser;
use opcua;
use opcua::server::address_space;
use opcua::server::{
    address_space::method::MethodBuilder, callbacks, prelude::*, session::SessionManager,
};
use opcua::sync::{Mutex, RwLock};
use opcua::trace_write_lock;
use rusqlite::{params, Connection, Result};
use std::path::PathBuf;
use std::sync::Arc;
use tokio;

mod watersensor;


#[derive(Parser)]
#[command(version = "0.1", about = "OPC UA server for a watersensor", long_about = None)]
struct Cli {
    #[arg(
        short,
        long,
        value_name = "FILE",
        default_value = "server.conf",
        help = "OPC UA server configuration file"
    )]
    config: PathBuf,
}

fn main() {
    let cli = Cli::parse();
    let server_config = ServerConfig::load(&PathBuf::from(cli.config)).unwrap();
    let server = Server::new(server_config);
    let address_space = server.address_space();

    // The address space is guarded so obtain a lock to change it
    {
        let mut address_space = address_space.write();
        let ns_idx = address_space.register_namespace("urn:watersensor.local");
        assert_eq!(ns_idx, Ok(2)); // Should match the code you generated!
        watersensor::populate_address_space(&mut address_space);
    }


    match server.address_space().try_write(){
        None => {println!("Address could not be locked")},
        _ => {println!("Address locked")}
    };
    server.run();
}