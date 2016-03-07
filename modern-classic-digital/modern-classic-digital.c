#include <pebble.h>
#include <time.h>

static Window* gWindow = 0;
static Layer* gComplicationLayer = 0;
static Layer* gDialLayer = 0;
static Layer* gInnerLayer = 0;

static struct TimeState
{
	// timeStyle will be parametric
	uint8_t 	hours, minutes, seconds,
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
	uint32_t	timeStyle,
				outerCircleOuterInset,
				outerCircleInnerInset,
				innerCircleOuterInset,
				innerCircleInnerInset,
				hourHandInset,
				hourHandWidth,
				minuteHandWidth,
				timeStyle2;

	GColor		backgroundColor,
				outerBackgroundColor;

} gTimeState = {0};

#define kSaveSizeHack ((((15 * sizeof(GColor)) + 3) & ~3) + 9 * sizeof(uint32_t) + 4)

enum
{
	kOptionDateLeadingZeroSuppression = (1 << 0),
	kOptionHourLeadingZeroSuppression = (1 << 1),
	kOption12HourTime = (1 << 2),
	kOptionHourHandSnap = (1 << 3),
	kOptionDemoMode = (1 << 4),
	kOptionHideWeekday = (1 << 5),
	kOptionHideMonth = (1 << 6),
	kOptionHideDate = (1 << 7),
	kOptionHideBattery = (1 << 8),
	kOptionHideConnectionLost = (1 << 9),
	kOptionHideDigitalTime = (1 << 10),
	kOptionVibrateOnDisconnect = (1 << 11),
};

enum
{
	kChargeStateCharging =	(1 << 0),
	kChargeStatePluggedIn =	(1 << 1),
};

enum
{
	kLanguageAutomatic = 0,		// (auto)
	kLanguageArabic = 1,		// ar
	kLanguageGerman = 2,		// de
	kLanguageEnglish = 3,		// en
	kLanguageSpanish = 4,		// es
	kLanguageFrench = 5,		// fr
	kLanguageItalian = 6,		// it
	kLanguageDutch = 7,			// nl
	kLanguagePortuguese = 8,	// pt
	kLanguageRussian = 9,		// ru
};

enum
{
	KEY_ELAPSED_OUTER_COLOR = 0,
	KEY_ELAPSED_OUTER_BACKGROUND = 1,
	KEY_ELAPSED_INNER_COLOR = 2,
	KEY_ELAPSED_INNER_BACKGROUND = 3,
	KEY_INNERMOST_BACKGROUND_COLOR = 4,
	KEY_INNERMOST_TEXT_COLOR = 5,
	KEY_HOUR_HAND_COLOR = 6,
	KEY_MINUTE_HAND_COLOR = 7,
	KEY_COMPLICATION_MONTH_COLOR = 8,
	KEY_COMPLICATION_DATE_COLOR = 9,
	KEY_COMPLICATION_DAY_COLOR = 10,
	KEY_COMPLICATION_BATTERY_COLOR = 11,
	KEY_COMPLICATION_BATTERY_ERROR_COLOR = 12,
	KEY_FLAGS = 13,
	KEY_FLAGS2 = 14,
	KEY_BACKGROUND_COLOR = 15,
	KEY_OUTER_BACKGROUND_COLOR = 16,
};


static char const* const kDaysOfWeek[] =
{
	"د", "ن", "ث", "ع", "خ", "ج", "س",					// kLanguageArabic = 1
	"SO.", "MO.", "DI.", "MI.", "DO.", "FR.", "SA.",	// kLanguageGerman = 2
	"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",	// kLanguageEnglish = 3
	"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB",	// kLanguageSpanish = 4
	"DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM",	// kLanguageFrench = 5
	"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB",	// kLanguageItalian = 6
	"ZON", "MAA", "DIN", "WOE", "DON", "VRI", "ZAT", 	// kLanguageDutch = 7
	"DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SÁB",	// kLanguagePortuguese = 8
	"ПНД", "ВТР", "СРД", "ЧТВ", "ПТН", "СБТ", "ВСК",	// kLanguageRussian = 9
	//"VSK", "PND", "VTR", "SRD", "CHT", "PTN", "SBT",	// kLanguageRussian = 9 (Romanized)
};

static char const* const kMonthNames[] =
{
	"ٌناٌر", "فبراٌر", "مارس", "إبرٌل", "ماٌو", "ٌونٌو", "ٌولٌو", "أؼسطس", "سبتمبر", "أكتوبر", "نوفمبر", "دٌسمبر",		// kLanguageArabic = 1	// TODO: Hijri calendar
	"JÄN", "FEB", "MÄR", "APR", "MAI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEZ", 	// kLanguageGerman = 2
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",		// kLanguageEnglish = 3
	"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC",		// kLanguageSpanish = 4
	"JAN", "FÉV", "MAR", "AVR", "MAI", "JUN", "JUL", "AOÛ", "SEP", "OCT", "NOV", "DÉC", 	// kLanguageFrench = 5
	"GEN", "FEB", "MAR", "APR", "MAG", "GIU", "LUG", "AGO", "SET", "OTT", "NOV", "DIC", 	// kLanguageItalian = 6
	"JAN", "FEB", "MAR", "APR", "MEI", "JUN", "JUL", "AUG", "SEP", "OKT", "NOV", "DEC",		// kLanguageDutch = 7
	"JAN", "FEV", "MAR", "ABR", "MAI", "JUN", "JUL", "AGO", "SET", "OUT", "NOV", "DEZ",		// kLanguagePortuguese = 8
	"янв", "фев", "мар", "апр", "май", "июн", "июл", "авг", "сен", "окт", "ноя", "дек",		// kLanguageRussian = 9
	//"IAN", "FEV", "MAR", "APR", "MAI", "IUN", "IUL", "AVG", "SEN", "OKT", "NOI", "DEK",	// kLanguageRussian = 9 (Romanized)
};

int automaticLanguage(void)
{
	char const* iso = i18n_get_system_locale();
	
	switch(iso[0])
	{
	case 'a':
		if(iso[1] == 'r')	return(kLanguageArabic);
		break;
	case 'd':
		if(iso[1] == 'e')	return(kLanguageGerman);
		break;
	case 'e':
		if(iso[1] == 'n')	return(kLanguageEnglish);
		if(iso[1] == 's')	return(kLanguageSpanish);
		break;
	case 'f':
		if(iso[1] == 'r')	return(kLanguageFrench);
		break;
	case 'i':
		if(iso[1] == 't')	return(kLanguageItalian);
		break;
	case 'n':
		if(iso[1] == 'l')	return(kLanguageDutch);
		break;
	case 'p':
		if(iso[1] == 't')	return(kLanguagePortuguese);
		break;
	case 'r':
		if(iso[1] == 'u')	return(kLanguageRussian);
		break;
	}

	return(kLanguageEnglish);	// fallback
}

// dayIndexZeroBased treats 0 as Sunday
static char const* localizedDayOfWeek(int dayIndexZeroBased)
{
	int lang = (gTimeState.timeStyle2 & 0xFF);
	if(lang == kLanguageAutomatic)
		lang = automaticLanguage();

	return(kDaysOfWeek[7 * (lang - 1) + dayIndexZeroBased]);
}

// monthIndexZeroBased treats 0 as January
static char const* localizedMonthName(int monthIndexZeroBased)
{
	int lang = gTimeState.timeStyle2 & 0xFF;
	if(lang == kLanguageAutomatic)
		lang = automaticLanguage();
	return(kMonthNames[12 * (lang - 1) + monthIndexZeroBased]);
}

static void updateTime(struct tm const* currentTime)
{
	if(gTimeState.timeStyle & kOptionDemoMode)
	{
		gTimeState.hours = 1;
		gTimeState.minutes = 50;
		gTimeState.seconds = 0;
		gTimeState.days = 4;
		gTimeState.months = 3;
		gTimeState.weekDay = 5;
	}
	else
	{
		gTimeState.hours = currentTime->tm_hour;
		gTimeState.minutes = currentTime->tm_min;
		gTimeState.seconds = currentTime->tm_sec;
		gTimeState.days = currentTime->tm_mday;
		gTimeState.months = currentTime->tm_mon;
		gTimeState.weekDay = currentTime->tm_wday;
	}
}

static void onTimeChanged(struct tm* currentTime, TimeUnits units)
{
	updateTime(currentTime);
	if((units & (DAY_UNIT | MONTH_UNIT)) && (gComplicationLayer != 0))
		layer_mark_dirty(gComplicationLayer);
	if(gDialLayer != 0)
		layer_mark_dirty(gDialLayer);
	if(gInnerLayer != 0)
		layer_mark_dirty(gInnerLayer);
}

static void onAccelerometerEvent(AccelAxisType axis, int32_t direction)
{
	;
}

static void onBatteryStatusChanged(BatteryChargeState charge)
{
	if(gTimeState.timeStyle & kOptionDemoMode)
	{
		gTimeState.chargeLevel = 70;
		gTimeState.chargeState = 0;
	}
	else
	{
		gTimeState.chargeLevel = charge.charge_percent;
		gTimeState.chargeState = (charge.is_charging? kChargeStateCharging : 0) | (charge.is_plugged? kChargeStatePluggedIn : 0);
	}

	if(gComplicationLayer != 0)
		layer_mark_dirty(gComplicationLayer);
}

static void onConnectionStatusChanged(bool connected)
{
	// on the lost connection edge, if this option is selected, vibrate
	if(!gTimeState.connectionLost && !connected && (gTimeState.timeStyle & kOptionVibrateOnDisconnect))
		vibes_double_pulse();

	gTimeState.connectionLost = !connected;
	if(gComplicationLayer != 0)
		layer_mark_dirty(gComplicationLayer);
}

static void onComplicationLayerRender(struct Layer* layer, GContext* context)
{
	GRect const bounds = layer_get_bounds(layer);
	GPoint const center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
	
	// paint the outer background color
	graphics_context_set_fill_color(context, gTimeState.outerBackgroundColor);
	graphics_fill_circle(context, center, (bounds.size.w / 2));
	
	// background
	graphics_context_set_fill_color(context, gTimeState.backgroundColor);
	graphics_fill_circle(context, center, (bounds.size.w / 2) - gTimeState.outerCircleOuterInset - 1);

	// draw the day (-of-week) complication, e.g. "TUE" for Tuesday.
	// the kDays string can/should be internationalized

	uint32_t	outerRadius = gTimeState.outerCircleOuterInset + gTimeState.outerCircleInnerInset + 2,
				innerRadius = gTimeState.innerCircleOuterInset - 2,
				midRadius = (outerRadius + innerRadius) / 2,
				w = 20, h = 14;

	if(!(gTimeState.timeStyle & kOptionHideWeekday))
	{
		char dayString[4];
		snprintf(dayString, 4, "%s", localizedDayOfWeek(gTimeState.weekDay));

		GRect dayBox = GRect((bounds.size.w / 2) - w, midRadius - h, 2 * w, 2 * h);
		graphics_context_set_text_color(context, gTimeState.complicationDayColor);
		graphics_draw_text(		context, dayString, fonts_get_system_font(FONT_KEY_GOTHIC_18), dayBox,
								GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
							);
	}
	

	// draw the month complication, e.g. "FEB" for February
	// the kMonths string can/should be internationalized
	if(!(gTimeState.timeStyle & kOptionHideMonth))
	{
		char monthString[4];

		snprintf(monthString, 4, "%s", localizedMonthName(gTimeState.months));

		GRect monthBox = GRect(outerRadius, (bounds.size.h / 2) - h, innerRadius - outerRadius, 2 * h);
		graphics_context_set_text_color(context, gTimeState.complicationMonthColor);
		graphics_draw_text(		context, monthString, fonts_get_system_font(FONT_KEY_GOTHIC_18),
								monthBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
							);
	}
	
	if(!(gTimeState.timeStyle & kOptionHideDate))
	{
		// draw the date (-of-month) complication, e.g. 29
		char dateString[3];

		// option: leading-zero suppression on date
		snprintf(dateString, 3, (gTimeState.timeStyle & kOptionDateLeadingZeroSuppression)? "%2i" : "%02i", gTimeState.days);

		GRect dateBox = GRect(bounds.size.w - innerRadius, (bounds.size.h / 2) - h - 3, innerRadius - outerRadius, 2 * h + 6);
		graphics_context_set_text_color(context, gTimeState.complicationDateColor);
		graphics_draw_text(		context, dateString, fonts_get_system_font(FONT_KEY_GOTHIC_24),
								dateBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
							);
	}

	char chargeString[5];
	int showBottomComplication = 0;
	// a lost connection is more important than the battery level, unless the battery level is really low
	if(gTimeState.connectionLost && (gTimeState.chargeLevel > 10))
	{
		if(!(gTimeState.timeStyle & kOptionHideConnectionLost))
		{
			showBottomComplication = 1;
			graphics_context_set_text_color(context, gTimeState.complicationBatteryErrorColor);
			snprintf(chargeString, 5, "CONN");
		}
	}
	else if(!(gTimeState.timeStyle & kOptionHideBattery))
	{
		showBottomComplication = 1;
		graphics_context_set_text_color(	context, (gTimeState.chargeLevel <= 20)? gTimeState.complicationBatteryErrorColor
													: gTimeState.complicationBatteryColor
										);
		
		if(!(gTimeState.chargeState & kChargeStateCharging) && (gTimeState.chargeState & kChargeStatePluggedIn))
			snprintf(chargeString, 5, "PLUG");
		else
			snprintf(chargeString, 5, "%3i%s", gTimeState.chargeLevel, (gTimeState.chargeState & kChargeStateCharging)? "+" : "%");
	}

	if(showBottomComplication)
	{
		GRect chargeBox = GRect((bounds.size.w / 2) - w, bounds.size.h - midRadius - h, 2 * w, 2 * h);
		graphics_draw_text(		context, chargeString, fonts_get_system_font(FONT_KEY_GOTHIC_18),
								chargeBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
							);
	}
}

static void onDialLayerRender(struct Layer* layer, GContext* context)
{
	GRect bounds = layer_get_bounds(layer);

	GPoint const center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

	uint32_t	hourAngle = ((gTimeState.hours >= 12)? (gTimeState.hours - 12) : gTimeState.hours) * 30,
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
	int pm = (gTimeState.hours >= 12);
	GRect innerCircle = GRect(		bounds.origin.x + gTimeState.innerCircleOuterInset, bounds.origin.y + gTimeState.innerCircleOuterInset,
									bounds.size.w - (2 * gTimeState.innerCircleOuterInset), bounds.size.h - (2 * gTimeState.innerCircleOuterInset)
								);
	graphics_context_set_fill_color(context, pm? gTimeState.elapsedInnerBackground : gTimeState.elapsedInnerColor);
	graphics_fill_radial(	context, innerCircle, GOvalScaleModeFillCircle,
							gTimeState.innerCircleInnerInset, 0, DEG_TO_TRIGANGLE(hourAngle)
						);
	graphics_context_set_fill_color(context, pm? gTimeState.elapsedInnerColor : gTimeState.elapsedInnerBackground);
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
	
	if(!(gTimeState.timeStyle & kOptionHideDigitalTime))
	{
		graphics_context_set_text_color(context, gTimeState.innermostTextColor);
	
		char timeString[6];
		int h = gTimeState.hours, m = gTimeState.minutes;

		// option: 12-hour time
		if((gTimeState.timeStyle & kOption12HourTime) && (gTimeState.hours > 12))
			h -= 12;
		
		// option: leading zero suppression: 0 = 09:41; 1 = 9:41
		char const* fmt = ((gTimeState.timeStyle & kOptionHourLeadingZeroSuppression) && (h < 10))? " %1i\n%02i" : "%02i\n%02i";
		
		snprintf(timeString, 6, fmt, h, m);

		graphics_draw_text(		context, timeString, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS),
								hourInnerBox, GTextOverflowModeWordWrap, GTextAlignmentCenter, 0
							);
	}
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
	if(persist_exists(0))
	{
		// @@temp hack
		persist_read_data(0, (void*)&(gTimeState.elapsedOuterColor), kSaveSizeHack);
	}
	else
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
		gTimeState.backgroundColor = GColorWhite;
		gTimeState.outerBackgroundColor = GColorWhite;

		gTimeState.outerCircleOuterInset = 12,
		gTimeState.outerCircleInnerInset = 8,
		gTimeState.innerCircleOuterInset = gTimeState.outerCircleOuterInset + 40,
		gTimeState.innerCircleInnerInset = 5,
		gTimeState.hourHandInset = gTimeState.outerCircleOuterInset + 25,
		gTimeState.hourHandWidth = 7,
		gTimeState.minuteHandWidth = 3;
	}
	
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

void onAppMessageReceived(DictionaryIterator* it, void* context)
{
	Tuple* tuple = dict_read_first(it);
	while(tuple != 0)
	{
		switch(tuple->key)
		{
		case KEY_ELAPSED_OUTER_COLOR:
			gTimeState.elapsedOuterColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_ELAPSED_OUTER_BACKGROUND:
			gTimeState.elapsedOuterBackground = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_ELAPSED_INNER_COLOR:
			gTimeState.elapsedInnerColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_ELAPSED_INNER_BACKGROUND:
			gTimeState.elapsedInnerBackground = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_INNERMOST_BACKGROUND_COLOR:
			gTimeState.innermostBackgroundColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_INNERMOST_TEXT_COLOR:
			gTimeState.innermostTextColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_HOUR_HAND_COLOR:
			gTimeState.hourHandColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_MINUTE_HAND_COLOR:
			gTimeState.minuteHandColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_COMPLICATION_MONTH_COLOR:
			gTimeState.complicationMonthColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_COMPLICATION_DATE_COLOR:
			gTimeState.complicationDateColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_COMPLICATION_DAY_COLOR:
			gTimeState.complicationDayColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_COMPLICATION_BATTERY_COLOR:
			gTimeState.complicationBatteryColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_COMPLICATION_BATTERY_ERROR_COLOR:
			gTimeState.complicationBatteryErrorColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_FLAGS:
			{
				unsigned int value = tuple->value->uint32;
				gTimeState.timeStyle = (value & 0xFFFF);
				gTimeState.hourHandWidth = ((value >> 16) & 0xFF);
				gTimeState.minuteHandWidth = ((value >> 24) & 0xFF);
			}
			break;
		case KEY_FLAGS2:
			gTimeState.timeStyle2 = tuple->value->uint32;
			break;
		case KEY_BACKGROUND_COLOR:
			gTimeState.backgroundColor = GColorFromHEX(tuple->value->uint32);
			break;
		case KEY_OUTER_BACKGROUND_COLOR:
			gTimeState.outerBackgroundColor = GColorFromHEX(tuple->value->uint32);
			break;
		}
		tuple = dict_read_next(it);
	}

	// @@temp hack
	persist_write_data(0, (void*)&(gTimeState.elapsedOuterColor), kSaveSizeHack);
	
	if(gDialLayer != 0)
		layer_mark_dirty(gDialLayer);
	if(gInnerLayer != 0)
		layer_mark_dirty(gInnerLayer);
	if(gComplicationLayer != 0)
		layer_mark_dirty(gComplicationLayer);
}

void onAppMessageDropped(AppMessageResult reason, void* context)
{
	// bummer
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

	//app_sync_init(struct AppSync * s, uint8_t * buffer, const uint16_t buffer_size, const Tuplet *const keys_and_initial_values, const uint8_t count, AppSyncTupleChangedCallback tuple_changed_callback, AppSyncErrorCallback error_callback, void * context)

	app_message_open(256, 64);
	app_message_register_inbox_received(&onAppMessageReceived);
	app_message_register_inbox_dropped(&onAppMessageDropped);
	
	window_stack_push(gWindow, true);
	app_event_loop();
}