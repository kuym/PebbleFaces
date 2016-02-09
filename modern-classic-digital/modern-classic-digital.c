#include <pebble.h>
#include <time.h>

static Window* gWindow = 0;
static Layer* gComplicationLayer = 0;
static Layer* gDialLayer = 0;
static Layer* gInnerLayer = 0;

static struct TimeState
{
	// timeStyle will be parametric
	uint8_t 	timeStyle,
				hours, minutes, seconds,
				months, days, weekDay,
				chargeLevel, chargeState,
				connectionLost;

	// all these colors will be parametric
	GColor		elapsedOuterColor,
				elapsedOuterBackground,
				elapsedInnerColor,
				elapsedInnerBackground,
				hourHandColor,
				minuteHandColor,
				innermostBackgroundColor,
				innermostTextColor,
				complicationMonthColor,
				complicationDateColor,
				complicationDayColor,
				complicationBatteryColor,
				complicationBatteryErrorColor;

	// these linear dimensions will also be parametric
	uint32_t	outerCircleOuterInset,
				outerCircleInnerInset,
				innerCircleOuterInset,
				innerCircleInnerInset,
				hourHandInset,
				hourHandWidth,
				minuteHandWidth;

} gTimeState = {0};

enum
{
	kOptionDateLeadingZeroSuppression = (1 << 0),
	kOptionHourLeadingZeroSuppression = (1 << 1),
	kOption12HourTime = (1 << 2),
	kOptionHourHandSnap = (1 << 3),
};

enum
{
	kChargeStateCharging =	(1 << 0),
	kChargeStatePluggedIn =	(1 << 1),
};

static void updateTime(struct tm const* currentTime)
{
	gTimeState.hours = currentTime->tm_hour;
	gTimeState.minutes = currentTime->tm_min;
	gTimeState.seconds = currentTime->tm_sec;
	gTimeState.days = currentTime->tm_mday;
	gTimeState.months = currentTime->tm_mon;
	gTimeState.weekDay = currentTime->tm_wday;
}

static void onTimeChanged(struct tm* currentTime, TimeUnits units)
{
	updateTime(currentTime);
	if(gComplicationLayer != 0)
		layer_mark_dirty(gComplicationLayer);
	if(gDialLayer != 0)
		layer_mark_dirty(gDialLayer);
	if((units & (DAY_UNIT | MONTH_UNIT)) && (gInnerLayer != 0))
		layer_mark_dirty(gInnerLayer);
}

static void onAccelerometerEvent(AccelAxisType axis, int32_t direction)
{
	;
}

static void onBatteryStatusChanged(BatteryChargeState charge)
{
	gTimeState.chargeLevel = charge.charge_percent;
	gTimeState.chargeState = (charge.is_charging? kChargeStateCharging : 0) | (charge.is_plugged? kChargeStatePluggedIn : 0);

	if(gComplicationLayer != 0)
		layer_mark_dirty(gComplicationLayer);
}

static void onConnectionStatusChanged(bool connected)
{
	gTimeState.connectionLost = !connected;
	if(gComplicationLayer != 0)
		layer_mark_dirty(gComplicationLayer);
}

static void onComplicationLayerRender(struct Layer* layer, GContext* context)
{
	// draw the day (-of-week) complication, e.g. "TUE" for Tuesday.
	// the kDays string can/should be internationalized
	GRect bounds = layer_get_bounds(layer);

	uint32_t	outerRadius = gTimeState.outerCircleOuterInset + gTimeState.outerCircleInnerInset + 3,
				innerRadius = gTimeState.innerCircleOuterInset - 3,
				midRadius = (outerRadius + innerRadius) / 2,
				w = 20, h = 14;

	char dayString[4];
	static char const kDays[] = "SUN" "MON" "TUE" "WED" "THU" "FRI" "SAT";

	snprintf(dayString, 4, "%s", kDays + (3 * gTimeState.weekDay));

	GRect dayBox = GRect((bounds.size.w / 2) - w, midRadius - h, 2 * w, 2 * h);
	graphics_context_set_text_color(context, gTimeState.complicationDayColor);
	graphics_draw_text(		context, dayString, fonts_get_system_font(FONT_KEY_GOTHIC_18), dayBox,
							GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
						);
	

	// draw the month complication, e.g. "FEB" for February
	// the kMonths string can/should be internationalized
	char monthString[4];
	static char const kMonths[] = "JAN" "FEB" "MAR" "APR" "MAY" "JUN" "JUL" "AUG" "SEP" "OCT" "NOV" "DEC";

	snprintf(monthString, 4, "%s", kMonths + (3 * gTimeState.months));

	GRect monthBox = GRect(outerRadius, (bounds.size.h / 2) - h, innerRadius - outerRadius, 2 * h);
	graphics_context_set_text_color(context, gTimeState.complicationMonthColor);
	graphics_draw_text(		context, monthString, fonts_get_system_font(FONT_KEY_GOTHIC_18),
							monthBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
						);

	
	// draw the date (-of-month) complication, e.g. 29
	char dateString[3];

	// option: leading-zero suppression on date
	snprintf(dateString, 3, (gTimeState.timeStyle & kOptionDateLeadingZeroSuppression)? "%2i" : "%02i", gTimeState.days);

	GRect dateBox = GRect(bounds.size.w - innerRadius, (bounds.size.h / 2) - h - 3, innerRadius - outerRadius, 2 * h + 6);
	graphics_context_set_text_color(context, gTimeState.complicationDateColor);
	graphics_draw_text(		context, dateString, fonts_get_system_font(FONT_KEY_GOTHIC_24),
							dateBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
						);
	

	char chargeString[5];

	// a lost connection is more important than the battery level, unless the battery level is really low
	if(gTimeState.connectionLost && (gTimeState.chargeLevel > 10))
	{
		graphics_context_set_text_color(context, gTimeState.complicationBatteryErrorColor);
		snprintf(chargeString, 5, "CONN");
	}
	else
	{
		graphics_context_set_text_color(	context, (gTimeState.chargeLevel <= 20)? gTimeState.complicationBatteryErrorColor
													: gTimeState.complicationBatteryColor
										);
		
		if(!(gTimeState.chargeState & kChargeStateCharging) && (gTimeState.chargeState & kChargeStatePluggedIn))
			snprintf(chargeString, 5, "PLUG");
		else
			snprintf(chargeString, 5, "%3i%s", gTimeState.chargeLevel, (gTimeState.chargeState & kChargeStateCharging)? "+" : "%");
	}

	GRect chargeBox = GRect((bounds.size.w / 2) - w, bounds.size.h - midRadius - h, 2 * w, 2 * h);
	graphics_draw_text(		context, chargeString, fonts_get_system_font(FONT_KEY_GOTHIC_18),
							chargeBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
						);
}

static void onDialLayerRender(struct Layer* layer, GContext* context)
{
	GRect bounds = layer_get_bounds(layer);

	GPoint const center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

	uint32_t	hourAngle = ((gTimeState.hours > 12)? (gTimeState.hours - 12) : gTimeState.hours) * 30,
				minuteAngle = gTimeState.minutes * 6;

	// option: hour hand snaps to hours rather than continuous movement
	if(!(gTimeState.timeStyle & kOptionHourHandSnap))
		hourAngle += (gTimeState.minutes / 2);
	
	// draw the outer angular section, which represents minutes
	GRect outerCircle = GRect(		bounds.origin.x + gTimeState.outerCircleOuterInset, bounds.origin.y + gTimeState.outerCircleOuterInset,
									bounds.size.w - (2 * gTimeState.outerCircleOuterInset), bounds.size.h - (2 * gTimeState.outerCircleOuterInset)
								);
	graphics_context_set_fill_color(context, gTimeState.elapsedOuterColor);
	graphics_fill_radial(	context, outerCircle, GOvalScaleModeFillCircle,
							gTimeState.outerCircleInnerInset, 0, DEG_TO_TRIGANGLE(minuteAngle)
						);
	graphics_context_set_fill_color(context, gTimeState.elapsedOuterBackground);
	graphics_fill_radial(	context, outerCircle, GOvalScaleModeFillCircle,
							gTimeState.outerCircleInnerInset, DEG_TO_TRIGANGLE(minuteAngle), DEG_TO_TRIGANGLE(360)
						);

	// draw the inner angular section, which represents hours
	GRect innerCircle = GRect(		bounds.origin.x + gTimeState.innerCircleOuterInset, bounds.origin.y + gTimeState.innerCircleOuterInset,
									bounds.size.w - (2 * gTimeState.innerCircleOuterInset), bounds.size.h - (2 * gTimeState.innerCircleOuterInset)
								);
	graphics_context_set_fill_color(context, gTimeState.elapsedInnerColor);
	graphics_fill_radial(	context, innerCircle, GOvalScaleModeFillCircle,
							gTimeState.innerCircleInnerInset, 0, DEG_TO_TRIGANGLE(hourAngle)
						);
	graphics_context_set_fill_color(context, gTimeState.elapsedInnerBackground);
	graphics_fill_radial(	context, innerCircle, GOvalScaleModeFillCircle,
							gTimeState.innerCircleInnerInset, DEG_TO_TRIGANGLE(hourAngle), DEG_TO_TRIGANGLE(360)
						);

	GRect hourCircle = GRect(	bounds.origin.x + gTimeState.hourHandInset, bounds.origin.y + gTimeState.hourHandInset,
								bounds.size.w - (2 * gTimeState.hourHandInset), bounds.size.h - (2 * gTimeState.hourHandInset)
							);
	GRect hourInnerCircle = GRect(		innerCircle.origin.x + gTimeState.innerCircleInnerInset, innerCircle.origin.y + gTimeState.innerCircleInnerInset,
										innerCircle.size.w - (2 * gTimeState.innerCircleInnerInset), innerCircle.size.h - (2 * gTimeState.innerCircleInnerInset)
									);
	// draw the hour hand
	graphics_context_set_stroke_color(context, gTimeState.hourHandColor);
	graphics_context_set_stroke_width(context, gTimeState.hourHandWidth);
	graphics_draw_line(		context, gpoint_from_polar(hourInnerCircle, GOvalScaleModeFillCircle, DEG_TO_TRIGANGLE(hourAngle)),
							gpoint_from_polar(hourCircle, GOvalScaleModeFillCircle, DEG_TO_TRIGANGLE(hourAngle))
						);
	
	// draw the minute hand
	graphics_context_set_stroke_color(context, gTimeState.minuteHandColor);
	graphics_context_set_stroke_width(context, gTimeState.minuteHandWidth);
	graphics_draw_line(		context, gpoint_from_polar(hourInnerCircle, GOvalScaleModeFillCircle, DEG_TO_TRIGANGLE(minuteAngle)),
							gpoint_from_polar(bounds, GOvalScaleModeFillCircle, DEG_TO_TRIGANGLE(minuteAngle))
						);

	// innermost circle
	graphics_context_set_fill_color(context, gTimeState.innermostBackgroundColor);
	graphics_fill_circle(context, center, hourInnerCircle.size.w / 2);
}

static void onInnerLayerRender(struct Layer* layer, GContext* context)
{
	GRect bounds = layer_get_bounds(layer);

	GPoint const center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

	uint32_t	innerCircleLayerInset = gTimeState.innerCircleOuterInset + gTimeState.innerCircleInnerInset + 1,
				innerRadius = center.x - innerCircleLayerInset;

	GRect hourInnerBox = GRect(		center.x - innerRadius, center.y - innerRadius,
									2 * innerRadius, 2 * innerRadius
								);
	
	graphics_context_set_text_color(context, gTimeState.innermostTextColor);
	
	char timeString[6];
	int h = gTimeState.hours, m = gTimeState.minutes;

	// option: 12-hour time
	if((gTimeState.timeStyle & kOption12HourTime) && (gTimeState.hours > 12))
		gTimeState.hours -= 12;
	
	// option: leading zero suppression: 0 = 09:41; 1 = 9:41
	char const* fmt = ((gTimeState.timeStyle & kOptionHourLeadingZeroSuppression) && (h < 10))? " %1i\n%02i" : "%02i\n%02i";
	
	snprintf(timeString, 6, fmt, h, m);

	graphics_draw_text(		context, timeString, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS),
							hourInnerBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
						);
}



static void onWindowLoad(Window* window)
{
	Layer* windowLayer = window_get_root_layer(window);

	GRect bounds = layer_get_bounds(windowLayer);

	// complications on bottom layer(s)
	gComplicationLayer = layer_create(bounds);
	layer_add_child(windowLayer, gComplicationLayer);
	layer_set_update_proc(gComplicationLayer, &onComplicationLayerRender);

	// then the dial/hands
	gDialLayer = layer_create(bounds);
	layer_add_child(windowLayer, gDialLayer);
	layer_set_update_proc(gDialLayer, &onDialLayerRender);

	// on the top is the inner circle layer
	gInnerLayer = layer_create(bounds);
	layer_add_child(windowLayer, gInnerLayer);
	layer_set_update_proc(gInnerLayer, &onInnerLayerRender);
}

static void onWindowAppear(Window* window)
{
	// initialize parameters to defaults
	gTimeState.elapsedOuterColor = GColorDarkCandyAppleRed;
	gTimeState.elapsedOuterBackground = GColorLightGray;
	gTimeState.elapsedInnerColor = GColorBlue;
	gTimeState.elapsedInnerBackground = GColorLightGray;
	gTimeState.hourHandColor = GColorBlue;
	gTimeState.minuteHandColor = GColorDarkCandyAppleRed;
	gTimeState.innermostBackgroundColor = GColorWhite;
	gTimeState.innermostTextColor = GColorBlack;
	gTimeState.complicationMonthColor = GColorDarkGray;
	gTimeState.complicationDateColor = GColorDarkGray;
	gTimeState.complicationDayColor = GColorDarkGray;
	gTimeState.complicationBatteryColor = GColorDarkGray;
	gTimeState.complicationBatteryErrorColor = GColorDarkCandyAppleRed;

	gTimeState.outerCircleOuterInset = 12,
	gTimeState.outerCircleInnerInset = 8,
	gTimeState.innerCircleOuterInset = gTimeState.outerCircleOuterInset + 40,
	gTimeState.innerCircleInnerInset = 5,
	gTimeState.hourHandInset = gTimeState.outerCircleOuterInset + 25,
	gTimeState.hourHandWidth = 7,
	gTimeState.minuteHandWidth = 3;
	
	// initialize to the current time
	time_t now;
	time(&now);
	updateTime(localtime(&now));

	// initialize battery level
	onBatteryStatusChanged(battery_state_service_peek());

	// initialize Bluetooth connection status
	gTimeState.connectionLost = !connection_service_peek_pebble_app_connection();
}

static void onWindowDisappear(Window* window)
{
	;	// needed for watchfaces?
}

static void onWindowUnload(Window* window)
{
	;	// needed for watchfaces?
}

int main(void)
{
	gWindow = window_create();

	WindowHandlers handlers =
	{
		.load = &onWindowLoad,
		.appear = &onWindowAppear,
		.disappear = &onWindowDisappear,
		.unload = &onWindowUnload,
	};
	window_set_window_handlers(gWindow, handlers);

	tick_timer_service_subscribe(YEAR_UNIT | MONTH_UNIT | DAY_UNIT | HOUR_UNIT | MINUTE_UNIT, &onTimeChanged);
	
	accel_tap_service_subscribe(&onAccelerometerEvent);

	battery_state_service_subscribe(&onBatteryStatusChanged);

	ConnectionHandlers connectionHandlers =
	{
		.pebble_app_connection_handler = &onConnectionStatusChanged,
		.pebblekit_connection_handler = 0,
	};
	connection_service_subscribe(connectionHandlers);
	
	window_stack_push(gWindow, true);
	app_event_loop();
}