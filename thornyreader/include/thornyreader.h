/*
 * Copyright (C) 2016 ThornyReader
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _THORNYREADER_H_
#define _THORNYREADER_H_

#define THORNYREADER_VERSION "1.0.6"

const char* ThornyReader_NDEBUG();
const char* ThornyReader_AXY_DEBUG();
void StartThornyReader(const char* name);

#define CONFIG_CRENGINE_FOOTNOTES               	100
/**
 * 0 - no antialias, 1 - antialias big fonts, 2 antialias all
 */
#define CONFIG_CRENGINE_FONT_ANTIALIASING       	101
#define CONFIG_CRENGINE_TXT_SMART_FORMAT 			102
#define CONFIG_CRENGINE_EMBEDDED_STYLES         	103
#define CONFIG_CRENGINE_EMBEDDED_FONTS          	104

#define CONFIG_CRENGINE_FONT_FACE_MAIN              105
#define CONFIG_CRENGINE_FONT_FACE_FALLBACK      	106
/**
 * Supported: 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.5, 1.9
 * See gammatbl.h
 */
#define CONFIG_CRENGINE_FONT_GAMMA              	107
#define CONFIG_CRENGINE_FONT_COLOR             		108
#define CONFIG_CRENGINE_FONT_SIZE               	109
/**
 * Supported: 80, 85, 90, 95, 100, 105, 110, 115, 120, 130, 140, 150, 160, 180, 200
 */
#define CONFIG_CRENGINE_INTERLINE_SPACE         	110
#define CONFIG_CRENGINE_BACKGROUND_COLOR        	111
/**
 * 1 for one-column mode, 2 for two-column mode in landscape orientation
 */
#define CONFIG_CRENGINE_PAGES_COLUMNS         		112
#define CONFIG_CRENGINE_MARGIN_TOP         			113
#define CONFIG_CRENGINE_MARGIN_BOTTOM      			114
#define CONFIG_CRENGINE_MARGIN_LEFT        			115
#define CONFIG_CRENGINE_MARGIN_RIGHT       			116
#define CONFIG_CRENGINE_VIEWPORT_WIDTH        		117
#define CONFIG_CRENGINE_VIEWPORT_HEIGHT     		118

/**
 * FORMAT_PDF 1, FORMAT_XPS 2
 */
#define CONFIG_MUPDF_FORMAT     		            201
#define CONFIG_MUPDF_INVERT_IMAGES 		            202

#define HARDCONFIG_DJVU_RENDERING_MODE              0
#define HARDCONFIG_MUPDF_SLOW_CMYK                  0

#endif /* _THORNYREADER_H_ */
