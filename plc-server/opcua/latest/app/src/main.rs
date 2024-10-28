use clap::Parser;
use opcua::server::{
    address_space::method::MethodBuilder, callbacks, prelude::*, session::SessionManager,
};
use opcua::sync::{Mutex, RwLock};
use rusqlite::{params, Result};
use std::path::PathBuf;
use std::sync::Arc;
use tokio;

struct GetTankSystemParamsMethod {
    db_conn: Arc<Mutex<rusqlite::Connection>>,
    fill_pct_node: NodeId,
    valve_pos_node: NodeId,
    threshold_node: NodeId,
    address_space: Arc<RwLock<AddressSpace>>,
}

impl callbacks::Method for GetTankSystemParamsMethod {
    fn call(
        &mut self,
        _session_id: &NodeId,
        _session_map: Arc<RwLock<SessionManager>>,
        _request: &CallMethodRequest,
    ) -> Result<CallMethodResult, StatusCode> {

        let conn = self.db_conn.lock();

        /*
         * Prepare statements and execute for FillPercentage
         */
        let fill_pct: f64 = {
            let mut stmt = match conn
                .prepare("SELECT level FROM waterlevel ORDER BY id DESC LIMIT 1;") {
                    Ok(statement) => statement,
                    Err(e) => {
                        eprintln!("Failed to prepare SQL statement for fill percentage with error: {:?}", e);
                        return Err(StatusCode::BadInternalError);
                    }
                };
            match stmt.query_row([], |row| row.get(0)) {
                Ok(level) => level,
                Err(rusqlite::Error::QueryReturnedNoRows) => {
                    eprintln!("No data found for fill percentage");
                    return Err(StatusCode::BadOutOfRange);
                }
                Err(e) => {
                    eprintln!("Error during query execution: {:?}", e);
                    return Err(StatusCode::BadInternalError);
                }
            }
        };

        /*
         * Prepare statements and execute for ValvePosition
         */
        let valve_pos: bool = {
            let mut stmt = match conn
                .prepare("SELECT position FROM valveposition ORDER BY id DESC LIMIT 1;") {
                    Ok(statement) => statement,
                    Err(e) => {
                        eprintln!("Failed to prepare SQL statement for valve position with error: {:?}", e);
                        return Err(StatusCode::BadInternalError);
                    }
                };
            match stmt.query_row([], |row| row.get(0)) {
                Ok(level) => level,
                Err(rusqlite::Error::QueryReturnedNoRows) => {
                    eprintln!("No data found for valve position");
                    return Err(StatusCode::BadOutOfRange);
                }
                Err(e) => {
                    eprintln!("Error during query execution: {:?}", e);
                    return Err(StatusCode::BadInternalError);
                }
            }
        };


        /*
         * Prepare statements and execute for Threshold
         */
        let threshold: i32 = {
            let mut stmt = match conn
                .prepare("SELECT threshold FROM triggerthreshold ORDER BY id DESC LIMIT 1;") {
                    Ok(statement) => statement,
                    Err(e) => {
                        eprintln!("Failed to prepare SQL statement for threshold with error: {:?}", e);
                        return Err(StatusCode::BadInternalError);
                    }
                };
            match stmt.query_row([], |row| row.get(0)) {
                Ok(level) => level,
                Err(rusqlite::Error::QueryReturnedNoRows) => {
                    eprintln!("No data found for threshold");
                    return Err(StatusCode::BadOutOfRange);
                }
                Err(e) => {
                    eprintln!("Error during query execution: {:?}", e);
                    return Err(StatusCode::BadInternalError);
                }
            }
        };

        /*
         * Write the retrieved values to the server
         * Async task is needed due to library limitations
         */
        let address_space = self.address_space.clone();
        let fill_pct_node = self.fill_pct_node.clone();
        let valve_pos_node = self.valve_pos_node.clone();
        let threshold_node = self.threshold_node.clone();

        tokio::spawn(async move {
            let now = DateTime::now();
            let mut address_space = address_space.write();
            address_space.set_variable_value(fill_pct_node, fill_pct, &now, &now);
            address_space.set_variable_value(valve_pos_node, valve_pos, &now, &now);
            address_space.set_variable_value(threshold_node, threshold, &now, &now);
        });

        /*
         * Return the retrieved values to client
         */
        Ok(CallMethodResult {
            status_code: StatusCode::Good,
            input_argument_results: None,
            input_argument_diagnostic_infos: None,
            output_arguments: Some(vec![fill_pct.into(), valve_pos.into(), threshold.into()]),
        })
    }
}


struct SetThresholdMethod {
    db_conn: Arc<Mutex<rusqlite::Connection>>,
    threshold_node: NodeId,
    address_space: Arc<RwLock<AddressSpace>>,
}
impl callbacks::Method for SetThresholdMethod {
    fn call(
        &mut self,
        _session_id: &NodeId,
        _session_map: Arc<RwLock<SessionManager>>,
        request: &CallMethodRequest,
    ) -> Result<CallMethodResult, StatusCode> {

        /*
         * Input validation
         */
        let in1_status = match request.input_arguments.as_deref() {
            Some([Variant::Int32(_)]) => StatusCode::Good,
            _ => return Err(StatusCode::BadInvalidArgument),
        };

        let new_threshold: i32 = match request.input_arguments.as_deref() {
            Some([Variant::Int32(val)]) => *val,
            _ => return Err(StatusCode::BadInvalidArgument),
        };

        let conn = self.db_conn.lock();

        /*
         * Prepare statements and execute for Threshold
         */
        let mut stmt = match conn.prepare("INSERT INTO triggerthreshold (threshold) VALUES (?);") {
            Ok(statement) => statement,
            Err(e) => {
                eprintln!("Failed to prepare SQL statement with error: {:?}", e);
                return Err(StatusCode::BadInternalError);
            }
        };

        match stmt.execute(params![new_threshold]) {
            Ok(_) => (),
            Err(e) => {
                eprintln!("Could not write threshold to database with error: {:?}", e);
                return Err(StatusCode::BadInternalError);
            }
        };

        /*
         * Write the new value to the server
         * Async task is needed due to library limitations
         */
        let address_space = self.address_space.clone();
        let threshold_node = self.threshold_node.clone();

        tokio::spawn(async move {
            let now = DateTime::now();
            let mut address_space = address_space.write();
            address_space.set_variable_value(threshold_node, new_threshold, &now, &now);
        });

        Ok(CallMethodResult {
            status_code: StatusCode::Good,
            input_argument_results: Some(vec![in1_status]),
            input_argument_diagnostic_infos: None,
            output_arguments: None,
        })
    }
}

#[derive(Parser)]
#[command(version = "0.1", about = "OPC UA server with encryption serving from a database", long_about = None)]
struct Cli {
    #[arg(
        short,
        long,
        value_name = "FILE",
        default_value = "server.conf",
        help = "OPC UA server configuration file"
    )]
    config: PathBuf,
    #[arg(
        short,
        long,
        value_name = "DATABASE",
        default_value = "db.sqlite",
        help = "SQLite database [default: /db.sqlite3]"
    )]
    database: PathBuf,
}

fn main() {
    let cli = Cli::parse();
    let server_config = ServerConfig::load(&PathBuf::from(cli.config)).unwrap();
    let server = Server::new(server_config);
    let address_space = server.address_space();

    /*
     * Create a connection to the database
     */
    let db_conn = Arc::new(Mutex::new(
        rusqlite::Connection::open(cli.database).unwrap(),
    ));

    /*
     * Prepare the server by defining object types and initializing the instance
     */
    let tank_system_type_ident = NodeId::next_numeric(1);
    {
        let mut address_space = address_space.write();

        ObjectTypeBuilder::new(
            &tank_system_type_ident,
            QualifiedName::new(1, "tankSystemType"),
            "tankSystemType",
        )
        .is_abstract(false)
        .subtype_of(ObjectTypeId::BaseObjectType)
        .insert(&mut address_space);

        let fill_pct_attr_ident = NodeId::next_numeric(1);
        VariableBuilder::new(
            &fill_pct_attr_ident,
            QualifiedName::new(1, "FillPercentage"),
            "FillPercentage",
        )
        .data_type(&DataTypeId::Double)
        .property_of(tank_system_type_ident.clone())
        .has_type_definition(VariableTypeId::PropertyType)
        .has_modelling_rule(ObjectId::ModellingRule_Mandatory)
        .value_rank(-1)
        .insert(&mut address_space);

        let valve_pos_attr_ident = NodeId::next_numeric(1);
        VariableBuilder::new(
            &valve_pos_attr_ident,
            QualifiedName::new(1, "ValvePosition"),
            "ValvePosition",
        )
        .data_type(&DataTypeId::Boolean)
        .property_of(tank_system_type_ident.clone())
        .has_type_definition(VariableTypeId::PropertyType)
        .has_modelling_rule(ObjectId::ModellingRule_Mandatory)
        .value_rank(-1)
        .insert(&mut address_space);

        let threshold_attr_ident = NodeId::next_numeric(1);
        VariableBuilder::new(
            &threshold_attr_ident,
            QualifiedName::new(1, "Threshold"),
            "Threshold",
        )
        .data_type(&DataTypeId::Int32)
        .property_of(tank_system_type_ident.clone())
        .has_type_definition(VariableTypeId::PropertyType)
        .has_modelling_rule(ObjectId::ModellingRule_Mandatory)
        .value_rank(-1)
        .insert(&mut address_space);

        let tank_system_1_ident = NodeId::next_numeric(1);
        ObjectBuilder::new(
            &tank_system_1_ident,
            QualifiedName::new(1, "tankSystem1"),
            "tankSystem1",
        )
        .organized_by(address_space.objects_folder().node_id())
        .has_type_definition(tank_system_type_ident.clone())
        .insert(&mut address_space);

        let fill_pct_ident = NodeId::next_numeric(1);
        VariableBuilder::new(
            &fill_pct_ident,
            QualifiedName::new(1, "FillPercentage"),
            "FillPercentage",
        )
        .data_type(&DataTypeId::Double)
        .property_of(tank_system_1_ident.clone())
        .has_type_definition(VariableTypeId::PropertyType)
        .value(Variant::Double(0.0))
        .insert(&mut address_space);

        let valve_pos_ident = NodeId::next_numeric(1);
        VariableBuilder::new(
            &valve_pos_ident,
            QualifiedName::new(1, "ValvePosition"),
            "ValvePosition",
        )
        .data_type(&DataTypeId::Boolean)
        .property_of(tank_system_1_ident.clone())
        .has_type_definition(VariableTypeId::PropertyType)
        .value(Variant::Boolean(false))
        .insert(&mut address_space);

        let threshold_ident = NodeId::next_numeric(1);
        VariableBuilder::new(
            &threshold_ident,
            QualifiedName::new(1, "Threshold"),
            "Threshold",
        )
        .data_type(&DataTypeId::Int32)
        .property_of(tank_system_1_ident.clone())
        .has_type_definition(VariableTypeId::PropertyType)
        .value(Variant::Int32(0))
        .insert(&mut address_space);

        /*
         * Add methods to server
         */
        let get_tank_system_params_method_ident = NodeId::next_numeric(1);
        MethodBuilder::new(
            &get_tank_system_params_method_ident,
            QualifiedName::new(1, "getTankSystemParams"),
            "getTankSystemParams",
        )
        .component_of(tank_system_1_ident.clone())
        .output_args(
            &mut address_space,
            &[
                ("FillPercentage", DataTypeId::Double).into(),
                ("ValvePosition", DataTypeId::Boolean).into(),
                ("Threshold", DataTypeId::Int32).into(),
            ],
        )
        .callback(Box::new(GetTankSystemParamsMethod {
            db_conn: db_conn.clone(),
            fill_pct_node: fill_pct_ident.clone(),
            valve_pos_node: valve_pos_ident.clone(),
            threshold_node: threshold_ident.clone(),
            address_space: server.address_space(),
        }))
        .insert(&mut address_space);

        let set_threshold_method_ident = NodeId::next_numeric(1);
        MethodBuilder::new(
            &set_threshold_method_ident,
            QualifiedName::new(1, "setThreshold"),
            "setThreshold",
        )
        .component_of(tank_system_1_ident.clone())
        .input_args(
            &mut address_space,
            &[("newThreshold", DataTypeId::Int32).into()],
        )
        .callback(Box::new(SetThresholdMethod {
            db_conn: db_conn.clone(),
            threshold_node: threshold_ident.clone(),
            address_space: server.address_space(),
        }))
        .insert(&mut address_space);
    }
    server.run();
}
