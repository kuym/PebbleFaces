Pebble.addEventListener("ready", function()
{
	console.log("PebbleKit JS ready!");
});

Pebble.addEventListener("showConfiguration", function()
{
	var url = "https://rawgit.com/kuym/PebbleFaces/master/modern-classic-digital/config/index.html";
	console.log("Showing configuration page: " + url);

	Pebble.openURL(url);
});

function parseColor(colorHex)
{
	if(colorHex.substr(0, 1) == "#")
		return(parseInt(colorHex.substr(1), 16));
	if(colorHex.substr(0, 2) == "0x")
		return(parseInt(colorHex.substr(2), 16));

	var c = parseInt(colorHex, 16);
	return(isNaN(c)? 0 : c);
}

Pebble.addEventListener("webviewclosed", function(e)
{
	var configData = JSON.parse(decodeURIComponent(e.response));
	console.log("Configuration page returned: " + JSON.stringify(configData));

	var flags = (parseInt(configData["minuteHandWidth_value"], 10) << 24)
		| ((parseInt(configData["hourHandWidth_value"], 10) << 16))
		| ((configData["optionDateLeadingZeroSuppression_option"]? 1 : 0) << 0)
		| ((configData["optionHourLeadingZeroSuppression_option"]? 1 : 0) << 1)
		| ((configData["option12HourTime_option"]? 1 : 0) << 2)
		| ((configData["optionHourHandSnap_option"]? 1 : 0) << 3)
		| ((configData["optionDemoMode_option"]? 1 : 0) << 4)
		| ((configData["optionHideWeekday_option"]? 1 : 0) << 5)
		| ((configData["optionHideMonth_option"]? 1 : 0) << 6)
		| ((configData["optionHideDate_option"]? 1 : 0) << 7)
		| ((configData["optionHideBattery_option"]? 1 : 0) << 8)
		| ((configData["optionHideConnectionLost_option"]? 1 : 0) << 9);
	
	var dict =
	{
		KEY_ELAPSED_OUTER_COLOR: parseColor(configData["elapsedOuterColor_picker"]),
		KEY_ELAPSED_OUTER_BACKGROUND: parseColor(configData["elapsedOuterBackground_picker"]),
		KEY_ELAPSED_INNER_COLOR: parseColor(configData["elapsedInnerColor_picker"]),
		KEY_ELAPSED_INNER_BACKGROUND: parseColor(configData["elapsedInnerBackground_picker"]),
		KEY_INNERMOST_BACKGROUND_COLOR: parseColor(configData["innermostBackgroundColor_picker"]),
		KEY_INNERMOST_TEXT_COLOR: parseColor(configData["innermostTextColor_picker"]),
		KEY_HOUR_HAND_COLOR: parseColor(configData["hourHandColor_picker"]),
		KEY_MINUTE_HAND_COLOR: parseColor(configData["minuteHandColor_picker"]),
		KEY_COMPLICATION_MONTH_COLOR: parseColor(configData["complicationMonthColor_picker"]),
		KEY_COMPLICATION_DATE_COLOR: parseColor(configData["complicationDateColor_picker"]),
		KEY_COMPLICATION_DAY_COLOR: parseColor(configData["complicationDayColor_picker"]),
		KEY_COMPLICATION_BATTERY_COLOR: parseColor(configData["complicationBatteryColor_picker"]),
		KEY_COMPLICATION_BATTERY_ERROR_COLOR: parseColor(configData["complicationBatteryErrorColor_picker"]),
		KEY_FLAGS: flags,
	};

	console.log("dict: " + JSON.stringify(dict));

	// Send to watchapp
	Pebble.sendAppMessage(dict, function()
	{
		console.log("Send successful: " + JSON.stringify(dict));
	}, function()
	{
		console.log("Send failed!");
	});
});