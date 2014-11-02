var lastResponse;

var TZOFF = -4 * 60 * 60;

function sendOneCalendar(i) {
	if (! lastResponse) return;
	if (i >= lastResponse.mtgs.length) return;
	
	//console.log("sendOneCalendar " + i);
	var o = lastResponse.mtgs[i];
	//console.log(JSON.stringify(o));
	Pebble.sendAppMessage({
		"calIndex": i,
		"calSubject": o.subject,
		"calStart": o.start+TZOFF,
		"calEnd": o.start+o.duration+TZOFF,
		"calLocation": o.location
	});
}

function sendCalendarSummary() {
	Pebble.sendAppMessage({
		"calTotal": lastResponse.mtgs.length
	});
}

function tryCalendar(i) {
	var req = new XMLHttpRequest();
	req.open('GET', "http://www.gutwin.org/ebw/biib.json?cache=" + (Math.random() * 100000), true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				console.log(req.responseText);
				lastResponse = JSON.parse(req.responseText);
				sendCalendarSummary();
			} else {
				console.log("Error " + req.status);
			}
		} else {
			console.log("calendar readyState " + req.readyState);
		}
	};
	req.timeout = 15000;
	req.ontimeout = function() {
		console.log("timeout");
	};
	req.send(null);
}


// Initialize application
Pebble.addEventListener(
	"ready",
	function(e) {
		console.log("CONNECTION ESTABLISHED " + e.ready);
	});

// Handle incoming AppMessages, dispatch to try* functions as appropriate.
Pebble.addEventListener(
	"appmessage",
	function(e) {
		//console.log("RECEIVED MESSAGE:");
		//console.log(JSON.stringify(e));
		if (e.payload.calFetchAll) {
			tryCalendar();
		}
		if (e.payload.calFetchOne) {
			sendOneCalendar(e.payload.calFetchOne - 1);
		}
	});

