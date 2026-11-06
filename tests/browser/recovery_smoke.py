# Optional end-to-end browser smoke. Run: python3 tests/browser/recovery_smoke.py
# Requires Playwright Python and a local Chromium installation.
import json, sys, threading
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Ground_Station'))
import ground_station
html=(ROOT/'Ground_Station/index.html').read_bytes()
state=ground_station.MANAGER.snapshot()
state.update(active_source='wifi',link_status='live',firmware_version='V12-development',recorder_mode=1,recording=False,sd_ok=True,battery_v=3.9,bench_recording=False,bench_max_ms=120000,log_file='')
state['sources']['wifi']['status']='live'
for key in ('firmware_version','recorder_mode','recording','sd_ok','battery_v','bench_recording','log_file'):
    state['field_meta'][key]={'freshness':'live','valid':True}
requests=[]
flight_sensor={"value":None}
imported_session={"value":None}
flight_details={"label":"","rocket_profile":"","flight_date":None}
timeline={'flight_id':'a'*64,'source':'onboard_rkt4','total_records':3,'displayed_records':3,'display_stride':1,'sensor_status':'valid','estimated_peak_index':1,'peak_vertical_speed_mps_estimated':12.0,'peak_acceleration_g':1.2,'warnings':[],
          'points':[{'index':0,'elapsed_s':0,'altitude_m_estimated':0,'vertical_speed_mps_estimated':None,'pressure_pa':101325,'temperature_c':20,'battery_v':4.1,'sensor':None},
                    {'index':1,'elapsed_s':.04,'altitude_m_estimated':10,'vertical_speed_mps_estimated':12,'pressure_pa':101000,'temperature_c':20,'battery_v':4.0,'sensor':{'gps_status':'live','latitude':37.1,'longitude':-121.9,'gps_altitude_m':100,'gps_speed_mps':5,'imu_valid':True,'acceleration_mps2':[1,2,3],'gyro_rad_s':[.1,.2,.3]}},
                    {'index':2,'elapsed_s':.08,'altitude_m_estimated':8,'vertical_speed_mps_estimated':-5,'pressure_pa':101050,'temperature_c':20,'battery_v':3.9,'sensor':None}]}
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path=='/': data,ctype=html,'text/html'
        elif self.path.startswith('/api/status'): data,ctype=json.dumps(state).encode(),'application/json'
        elif self.path=='/api/ports': data,ctype=json.dumps({'ports':[],'config':{'serial_port':'auto','feather_url':'http://192.168.4.1'}}).encode(),'application/json'
        elif self.path.startswith('/api/flights/compare?'): data,ctype=json.dumps({'gps_note':'Last current GPS fix is not a confirmed landing position.','flights':[{'label':'Recovery test','rocket_profile':'Estes So Long','flight_date':'2026-11-05','peak_barometric_altitude_m_estimated':10,'duration_s':.08,'peak_vertical_speed_mps_estimated':12,'peak_imu_magnitude_g':1.2,'battery_start_v':4.1,'battery_end_v':3.9,'last_current_gps_fix':[37.1,-121.9],'sensor_status':'valid','rkt4_warnings':[],'sensor_sequence_gaps':0,'sha256':'a'*64},{'label':'Second flight','rocket_profile':None,'flight_date':None,'peak_barometric_altitude_m_estimated':8,'duration_s':.08,'peak_vertical_speed_mps_estimated':None,'peak_imu_magnitude_g':None,'battery_start_v':4,'battery_end_v':3.8,'last_current_gps_fix':None,'sensor_status':'missing','rkt4_warnings':['Torn tail'],'sensor_sequence_gaps':0,'sha256':'b'*64}]}).encode(),'application/json'
        elif self.path=='/api/flights': data,ctype=json.dumps({'flights':[{'id':'a'*64,'original_name':'V12D007.BIN','sha256':'a'*64,'summary':{'records':3,'duration_s':.08,'peak_altitude_m_estimated':1.0,'bytes':80,'warnings':[]},**flight_details,**({'sensor':flight_sensor['value']} if flight_sensor['value'] else {})}]}).encode(),'application/json'
        elif self.path.startswith('/api/flight/') and self.path.endswith('/timeline'): data,ctype=json.dumps(timeline).encode(),'application/json'
        elif self.path=='/api/sessions': data,ctype=json.dumps({'sessions':[imported_session['value']] if imported_session['value'] else []}).encode(),'application/json'
        else: data,ctype=b'{}','application/json'
        self.send_response(200);self.send_header('Content-Type',ctype);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    def do_POST(self):
        if self.path=='/api/import-session':
            body=self.rfile.read(int(self.headers.get('Content-Length',0)));requests.append((self.path,body,self.headers.get('X-Green-Ion-Filename')))
            inspection={'records':1,'source_counts':{'wifi':1,'radio':0,'simulation':0},'out_of_order_receive_times':0,'final_newline':True}
            imported_session['value']={'name':'session-import-'+('c'*64)+'.jsonl','bytes':len(body),'modified':'2026-11-06T03:30:00Z','source_kind':'imported_jsonl','original_name':'field-session.jsonl','sha256':'c'*64,'inspection':inspection}
            data=json.dumps({'success':True,'session':{'duplicate':False,**imported_session['value']}}).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        if self.path.startswith('/api/import-sen/'):
            body=self.rfile.read(int(self.headers.get('Content-Length',0)));requests.append((self.path,body,self.headers.get('X-Green-Ion-Filename')))
            flight_sensor['value']={'original_name':'V12D007.SEN','sha256':'b'*64,'inspection':{'status':'valid','valid_prefix_records':1,'sequence_gaps':0}}
            data=json.dumps({'success':True,'sensor':flight_sensor['value']}).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        body=json.loads(self.rfile.read(int(self.headers.get('Content-Length',0))) or b'{}');requests.append((self.path,body))
        if self.path.endswith('/details'): flight_details.update(body)
        if self.path=='/api/bench/start': state.update(recorder_mode=2,recording=True,bench_recording=True,log_file='/TEST000.BIN')
        if self.path=='/api/bench/stop': state.update(recorder_mode=3,recording=False,bench_recording=False)
        data=json.dumps({'success':True,'file':state['log_file'],'status':'PASS' if self.path=='/api/bench/verify' else None,'message':'RKT4 file validated; physical disconnect unverified' if self.path=='/api/bench/verify' else 'OK'}).encode()
        self.send_response(202);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    def log_message(self,*args): pass
server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
with sync_playwright() as p:
    browser=p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':390,'height':844})
    errors=[];page.on('pageerror',lambda e:errors.append(e.stack))
    page.on('dialog',lambda d:d.accept('START BENCH TEST' if d.type=='prompt' and 'START BENCH' in d.message else 'STOP BENCH TEST' if d.type=='prompt' else None))
    page.goto(f'http://127.0.0.1:{server.server_port}/');page.click('nav button[data-view=preflight]')
    page.wait_for_function("!document.querySelector('#startBench').disabled")
    page.evaluate("renderPreflight({run_status:'COMPLETE',overall:'WARNING',generated_utc:'2026-11-05T14:00:00Z',checks:[{name:'SD initialization',status:'FAIL',message:'SD fault',correction:'Inspect SD wiring.'}]})")
    assert 'Next: Inspect SD wiring.' in page.locator('#preflightReport').inner_text()
    page.click('#startBench');page.wait_for_function("!document.querySelector('#stopBench').disabled")
    assert requests[-1]==('/api/bench/start',{'confirm':'START BENCH TEST'}),requests
    state.update(active_source='radio',link_status='live');state['field_meta']['bench_recording']['freshness']='stale'
    page.wait_for_function("document.querySelector('#benchStage').textContent.includes('unverified')",timeout=6000)
    assert page.locator('#stopBench').is_disabled()
    state.update(active_source='wifi',link_status='live');state['field_meta']['bench_recording']['freshness']='live'
    page.wait_for_function("!document.querySelector('#stopBench').disabled",timeout=6000)
    page.click('#stopBench');page.wait_for_function("document.querySelector('#benchStage').textContent.includes('No active bench')",timeout=6000)
    assert requests[-1]==('/api/bench/stop',{'confirm':'STOP BENCH TEST','file':'/TEST000.BIN'}),requests
    page.wait_for_function("!document.querySelector('#verifyBench').disabled")
    page.click('#verifyBench')
    page.wait_for_function("document.querySelector('#benchResult').textContent.includes('RKT4 file validated')")
    assert requests[-1]==('/api/bench/verify',{'file':'/TEST000.BIN'}),requests
    page.click('nav button[data-view=diagnostics]')
    assert page.locator('[data-diagnostic]').count()==22
    before_demo=len(requests)
    page.click('#playDiagnosticSimulation')
    page.wait_for_function("document.querySelector('#diagnosticSimulation').textContent.includes('ASCENT')")
    assert 'SIMULATION — NOT HARDWARE' in page.locator('#diagnosticSimulation').inner_text()
    assert len(requests)==before_demo
    state.update(last_position=[37.1,-121.9],last_position_phase=5,last_position_fix_quality=1,last_position_satellites=7,last_position_utc='2026-11-05T19:00:00Z',last_position_age_ms=10000,flight_state=6,gps_status='stale')
    page.click('nav button[data-view=recovery]')
    assert not page.locator('#rocketWiFiReturn').is_hidden()
    assert not page.locator('#recoveryInspectSD').is_disabled()
    assert page.locator('#recoveryDistance').inner_text()=='—'
    page.wait_for_function("document.querySelector('#positionConfidence').textContent.includes('not a confirmed landing')")
    assert 'unconfirmed' in page.locator('#landingEstimate').inner_text().lower()
    state['last_position_phase']=6
    page.wait_for_function("document.querySelector('#positionConfidence').textContent.includes('landing location still needs confirmation')")
    assert page.locator('#googleRocket').get_attribute('href')=='https://www.google.com/maps/search/?api=1&query=37.1%2C-121.9'
    assert page.evaluate('directDistanceBearing([0,0],[0,1]).meters')>111000
    assert abs(page.evaluate('directDistanceBearing([0,0],[0,1]).bearing')-90)<.01
    assert page.evaluate('directDistanceBearing([0,179.5],[0,-179.5]).meters')<112000
    assert page.evaluate('directDistanceBearing([0,0],[0,0]).bearing') is None
    assert page.evaluate('directDistanceBearing([91,0],[0,0])') is None
    page.fill('#manualLatitude','37.1');page.fill('#manualLongitude','-121.89');page.click('#setManualLocation')
    assert 'm' in page.locator('#recoveryDistance').inner_text()
    assert '270° true' in page.locator('#recoveryBearing').inner_text()
    page.click('#loadRecoveryMap')
    page.wait_for_function('document.querySelectorAll("#recoveryMapOverlay [data-marker]").length===2')
    assert page.locator('#recoveryMapOverlay [data-marker=rocket]').count()==1
    assert page.locator('#recoveryMapOverlay [data-marker=user]').count()==1
    page.evaluate("document.querySelector('#recoveryMapTiles img').dispatchEvent(new Event('error'))")
    assert 'imagery unavailable' in page.locator('#recoveryMapStatus').inner_text()
    assert page.locator('#recoveryDistance').inner_text()!='—'
    state.update(active_source='radio',gps_status='stale',link_status='live')
    state['sources']['wifi']['status']='stale'
    page.wait_for_function('document.querySelector("#recoveryWiFi").textContent.includes("Disconnected")')
    assert page.locator('#recoveryInspectSD').is_disabled()
    assert not page.locator('#rocketWiFiReturn').is_visible()
    state['sources']['radio']['status']='missing'
    state.update(active_source=None,link_status='stale',last_position_age_ms=60000)
    page.wait_for_function('document.querySelector("#recoveryDistance").textContent!=="—" && document.querySelector("#gpsStatus").textContent==="STALE"')
    page.wait_for_function('document.querySelector("#positionDetail").textContent.includes("60.0 s")')
    assert page.locator('#coords').inner_text()=='37.100000, -121.900000'
    state.update(active_source='wifi',gps_status='live',link_status='live')
    state['sources']['wifi']['status']='live'
    page.wait_for_function('document.querySelector("#rocketWiFiReturn").offsetParent!==null')
    assert not page.locator('#recoveryInspectSD').is_disabled()
    page.click('nav button[data-view=settings]');page.select_option('#recoveryUnits','imperial')
    page.click('nav button[data-view=recovery]')
    assert 'ft' in page.locator('#recoveryDistance').inner_text()
    page.fill('#manualLatitude','95');page.click('#setManualLocation')
    assert 'valid latitude' in page.locator('#userPositionStatus').inner_text()
    page.wait_for_timeout(700)
    assert 'valid latitude' in page.locator('#userPositionStatus').inner_text()
    page.context.grant_permissions(['geolocation'],origin=f'http://127.0.0.1:{server.server_port}')
    page.context.set_geolocation({'latitude':37.1,'longitude':-121.88,'accuracy':15})
    page.click('#useDeviceLocation')
    page.wait_for_function('document.querySelector("#userPositionStatus").textContent.startsWith("device ·")')
    assert 'accuracy ±15 m' in page.locator('#userPositionStatus').inner_text()
    page.click('#useDeviceLocation')
    page.evaluate('navigator.geolocation.watchPosition=(success,error)=>{window.geoSuccess=success;window.geoError=error;return 42}')
    page.click('#useDeviceLocation')
    page.evaluate('window.geoSuccess({coords:{latitude:37.1,longitude:-121.88,accuracy:15},timestamp:Date.now()})')
    first_distance=page.locator('#recoveryDistance').inner_text()
    page.evaluate('window.geoError({code:3,message:""})')
    assert 'temporarily unavailable' in page.locator('#userPositionStatus').inner_text()
    assert page.locator('#useDeviceLocation').inner_text()=='Stop device location'
    page.evaluate('window.geoSuccess({coords:{latitude:37.1,longitude:-121.87,accuracy:15},timestamp:Date.now()})')
    assert page.locator('#recoveryDistance').inner_text()!=first_distance
    assert 'temporarily unavailable' not in page.locator('#userPositionStatus').inner_text()
    assert page.locator('#recoveryMapOverlay [data-marker=user]').count()==1
    state.update(last_position=[95,-121.9])
    page.wait_for_function('document.querySelector("#coords").textContent==="No valid position"')
    assert page.locator('#googleRocket').is_hidden()
    state.update(last_position=None)
    page.wait_for_function('document.querySelector("#googleRocket").hidden')
    assert page.locator('#recoveryDistance').inner_text()=='—'
    page.click('nav button[data-view=analysis]')
    page.wait_for_function('document.querySelectorAll("[data-sensor-import]").length===1')
    assert page.locator('[data-sensor-fetch]').count()==1
    assert page.locator('[data-sensor-fetch]').is_enabled()
    page.locator('[data-sensor-file]').set_input_files({'name':'V12D007.SEN','mimeType':'application/octet-stream','buffer':b'GIS1'})
    page.click('[data-sensor-import]')
    page.wait_for_function('document.querySelectorAll("a[href$=sensor-raw]").length===1')
    assert requests[-1]==('/api/import-sen/'+'a'*64,b'GIS1','V12D007.SEN')
    assert 'GIS1 valid' in page.locator('#flightLibrary').inner_text()
    page.fill('#flightSearch','no matching rocket')
    assert 'No flights match' in page.locator('#flightLibrary').inner_text()
    page.fill('#flightSearch','V12D007')
    page.select_option('#flightSensorFilter','attached')
    assert page.locator('[data-flight-row]').count()==1
    page.select_option('#flightSourceFilter','feather_sd')
    assert page.locator('[data-flight-row]').count()==0
    page.select_option('#flightSourceFilter','unknown')
    page.select_option('#flightIntegrityFilter','clean')
    assert page.locator('[data-flight-row]').count()==1
    page.locator('[data-flight-row] details summary').click()
    page.fill('[data-detail-label]','Recovery test')
    page.fill('[data-detail-rocket]','Estes So Long')
    page.fill('[data-detail-date]','2026-11-05')
    page.click('[data-details-save]')
    page.wait_for_function('document.querySelector("[data-flight-row] strong").textContent==="Recovery test"')
    assert 'Estes So Long' in page.locator('#flightLibrary').inner_text()
    page.click('[data-analyze]')
    page.wait_for_function('document.querySelector("#analysisSummary").textContent.includes("3 display points")')
    assert 'ONBOARD REPLAY' in page.locator('#analysisReadout').inner_text()
    page.locator('#analysisScrub').fill('1')
    assert 'estimated apogee sample' in page.locator('#analysisReadout').inner_text()
    assert '37.100000' in page.locator('#analysisReadout').inner_text()
    page.select_option('#analysisSeries','gps_altitude_m')
    assert '100.00' in page.locator('#analysisReadout').inner_text()
    page.locator('#analysisScrub').fill('2')
    assert 'missing' in page.locator('#analysisReadout').inner_text()
    page.select_option('#analysisSpeed','16')
    page.click('#analysisPlay')
    page.wait_for_function('document.querySelector("#analysisPlay").textContent==="Play replay"')
    page.fill('#flightSearch','')
    page.select_option('#flightSourceFilter','all')
    page.select_option('#flightSensorFilter','all')
    page.evaluate("allFlights.push({...allFlights[0],id:'b'.repeat(64),original_name:'AIRP008.BIN',sha256:'b'.repeat(64),label:'Second flight',sensor:null});renderFlightLibrary()")
    page.locator('[data-compare-select]').nth(0).check()
    page.locator('[data-compare-select]').nth(1).check()
    assert page.locator('#compareFlights').is_enabled()
    page.click('#compareFlights')
    page.wait_for_function('document.querySelector("#flightComparison").textContent.includes("Second flight")')
    assert 'not a confirmed landing' in page.locator('#flightComparison').inner_text()
    raw_session=b'{"received_utc":"2026-11-06T03:30:00Z","source":"wifi","data":{}}\n'
    page.locator('#sessionFile').set_input_files({'name':'field-session.jsonl','mimeType':'application/x-ndjson','buffer':raw_session})
    page.click('#importSession')
    page.wait_for_function('document.querySelector("#sessionImportStatus").textContent.includes("Imported · 1 events")')
    assert requests[-1]==('/api/import-session',raw_session,'field-session.jsonl')
    assert 'Imported field-session.jsonl' in page.locator('#sessions').inner_text()
    assert page.locator('#sessions a').get_attribute('href').endswith('.jsonl')
    assert not errors,errors
    assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth')
    print('Recovery browser smoke passed: bench source return, location, map, units, offline fallback, mobile width')
    browser.close()
server.shutdown()
