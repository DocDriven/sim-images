from dash import Dash, html, dcc, callback, Output, Input, State
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import sqlite3
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

DB_PATH = '/usr/src/app/data.sqlite3'
INTERVAL = 500

app = Dash()

app.layout = html.Div(
  children=[
    html.H1(children='Tank supervisor', style={'textAlign':'center', 'color':'darkgray', 'fontSize':30}),
    html.Hr(),
    html.Div(
      children=[
        dcc.Graph(figure={}, id='graph'),
        dcc.Interval(id='current-refresh', interval=INTERVAL, n_intervals=0), # in milliseconds
      ]
    ),
    html.Hr(),
    html.Div(
      children=[
        html.Div(
          children=[
            html.Div(
              children=[
                html.H4(children='Level:', id='level-title', style={'margin-left': '5px', 'margin-right': '5px'}),
                html.Div(children='', id='level-current', style={'display':'inline-block','width':'150px'})
              ],
              style={'display':'flex', 'flex-direction':'row','align-items':'center', 'max-width':'200px'}
            ),
            # dcc.Input(value=0, type='number', id='level-input', disabled=True),
            # html.Button(children='Set', id='level-button', disabled=True)
          ]
        ),
        html.Div(
          children=[
            html.Div(
              children=[
                html.H4(children='Threshold:', id='threshold-title', style={'margin-left': '5px', 'margin-right': '5px'}),
                html.Div(children='', id='threshold-current', style={'display':'inline-block','width':'150px'})
              ],
              style={'display':'flex', 'flex-direction':'row','align-items':'center', 'max-width':'200px'}
            ),
            dcc.Input(value=0, type='number', id='threshold-input', style={'max-width':'75px'}),
            html.Button(children='Set', id='threshold-button'),
            html.Div(id='hidden-div', style={'display':'none'})
          ]
        ),
        html.Div(
          children=[
            html.Div(
              children=[
                html.H4(children='Valve Position:', id='position-title', style={'margin-left': '5px', 'margin-right': '5px'}),
                html.Div(children='', id='position-current', style={'display':'inline-block','width':'150px'})
              ],
              style={'display':'flex', 'flex-direction':'row','align-items':'center', 'max-width':'200px'}
            )
          ]
        ),
      ],
      style={'display':'flex', 'flex-direction':'row', 'flex-wrap':'wrap', 'align-content':'center','align-items':'flex-start','justify-content':'center'}
    )
  ]
)


@callback(
    [Output(component_id='graph', component_property='figure'),
     Output(component_id='level-current', component_property='children'),
     Output(component_id='threshold-current', component_property='children'),
     Output(component_id='position-current', component_property='children')],
    [Input(component_id='current-refresh', component_property='n_intervals')],
    prevent_initial_call=True,
)
def refresh_graph(n_intervals):
    curr_lvl = 0
    curr_thd = 0
    curr_pos = 0
    fig = go.Figure()
    try: 
        with sqlite3.connect(DB_PATH) as db:
            fig = make_subplots(rows=3, cols=1, shared_xaxes=True, vertical_spacing=0.02)
            data = pd.read_sql(f'SELECT timestamp, level, position, threshold FROM tanksystemdata ORDER BY id DESC LIMIT 250',
                               con=db, columns=['timestamp', 'level', 'position', 'threshold'])

            fig.append_trace(go.Scatter(x=data['timestamp'], y=data['level'], mode='lines+markers', name='Level'), row=1,col=1)
            curr_lvl = data['level'][0]

            fig.append_trace(go.Scatter(x=data['timestamp'], y=data['threshold'], mode='lines+markers', name='Threshold'), row=2,col=1)
            curr_thd = data['threshold'][0]

            fig.append_trace(go.Scatter(x=data['timestamp'], y=data['position'], mode='lines+markers', name='Valve Position'), row=3,col=1)
            curr_pos = data['position'][0]

            fig.update_layout(title_text="Current Level, threshold & valve position")

    except sqlite3.OperationalError as e:
        logger.error(f'Error connecting to the database: {e}')
    except sqlite3.DatabaseError as e:
        logger.error(f'Error during database operation: {e}')
    except Exception as e:
        logger.error(f'Error unexpected exception: {e}')

    if curr_pos == 0:
        curr_pos = "Closed"
    else:
        curr_pos = "Open"
    return fig, curr_lvl, curr_thd, curr_pos


@callback(
    Output(component_id='hidden-div', component_property='children'),
    Input(component_id='threshold-button', component_property='n_clicks'),
    State(component_id='threshold-input', component_property='value'),
    prevent_initial_call=True
)
def set_threshold(n_clicks, value):
    if value is None:
        logger.warning(f'No value given, exiting.')
        return
    try:
        if not isinstance(value, int):
            logger.warning(f'No integer given, exiting.')
            return

        with sqlite3.connect(DB_PATH) as db:
            cursor = db.cursor()
            cursor.execute(f"INSERT INTO triggerthreshold (threshold,) VALUES (?,);", (value,))
            db.commit()
    except sqlite3.OperationalError as e:
        logger.error(f'Error connecting to the database: {e}')
    except sqlite3.DatabaseError as e:
        logger.error(f'Error during database operation: {e}')
    except Exception as e:
        logger.error(f'Error unexpected exception: {e}')


if __name__ == '__main__':
    app.run_server(host="0.0.0.0", debug=True, port=8050)
