<?php

/**
 * @file
 * Provide forms for configuration of weather displays.
 * 
 * Copyright © 2006-2010 Tobias Quathamer <t.quathamer@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * Show an overview of configured displays and the default display.
 */
function weather_admin_main_page() {
  $output = '';
  $path = 'admin/config/user-interface/weather/system-wide/';
  $displays = weather_get_displays_in_use('system-wide');
  if (!empty($displays)) {
    foreach ($displays as $display_number) {
      $header = array(
        l(t('System-wide display (number !number)', array('!number' => $display_number)), $path . $display_number),
        t('Weight'),
      );

      $rows = array();
      $result = db_query("SELECT * FROM {weather_location}
        WHERE display_type='system-wide' AND display_number=:number ORDER BY weight ASC, real_name ASC",
        array(':number' => $display_number));
      foreach ($result as $location) {
        $rows[] = array(
          l($location->real_name, $path . $display_number . '/' . $location->id . '/edit'),
          $location->weight,
        );
      }
      // Insert link for adding locations into the table as last row.
      $rows[] = array(array(
        'data' => l(t('Add location to this display'), $path . $display_number . '/add'),
        'colspan' => 2,
      ));

      $output .= theme('table', array('header' => $header, 'rows' => $rows));
      if (isset($form['pager']['#value'])) {
        $output .= drupal_render($form['pager']);
      }
    }
  }
  $form = drupal_get_form('weather_admin_main_page_form');
  $output .= drupal_render($form);
  return $output;
}

/**
 * Construct a form for general settings of the Weather module
 */
function weather_admin_main_page_form() {
  $form['weather_image_directory'] = array(
    '#type' => 'textfield',
    '#title' => t('Directory for custom images'),
    '#description' => t('Override the default image directory. This directory must be a subdirectory of the Drupal \'files\' path.'),
    '#default_value' => variable_get('weather_image_directory', ''),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  return $form;
}

/**
 * Handle the submission for general settings of the Weather module
 */
function weather_admin_main_page_form_submit($form, &$form_state) {
  $directory = $form_state['values']['weather_image_directory'];
  // Remove whitespace and directory separators from the string.
  $directory = trim(trim($directory, '/\\'));
  // Replace Windows-style directory separators with Unix separators
  $directory = implode('/', explode('\\', $directory));
  variable_set('weather_image_directory', $directory);
  drupal_set_message(t('The configuration has been saved.'));
}

/**
 * Show an overview of configured locations for a user.
 *
 * @param User ID.
 */
function weather_user_main_page($uid) {
  $header = array(
    t('Real name'),
    t('Weight')
  );
  $path = 'user/' . $uid . '/weather/';
  $rows = array();
  $result = db_query('SELECT * FROM {weather_location}
    WHERE display_type=:type AND display_number=:number ORDER BY weight ASC, real_name ASC',
    array(':type' => 'user', ':number' => $uid));
  foreach ($result as $location) {
    $rows[] = array(
      l($location->real_name, $path . $location->id),
      $location->weight
    );
  }
  // Insert link for adding locations into the table as last row.
  $rows[] = array(array(
    'data' => l(t('Add location to this display'), $path . '/add'),
    'colspan' => 2,
  ));
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  if (isset($form['pager']['#value'])) {
    $output .= drupal_render($form['pager']);
  }
  $output .= '<p>' . l(t('Edit configuration of display'),
    $path . 'display') . '</p>';
  return $output;
}

/**
 * Create a settings form for a weather display.
 *
 * @param Type of the display (e.g. system-wide, user, location, ...).
 * @param Number of the display.
 *
 * @return Form array.
 */
function weather_display_settings_form($form, &$form_state, $display_type, $display_number=NULL) {
  $mode = 'edit';
  if ($display_number == 'add') {
    // Preserve the mode for this form.
    $mode = 'add';
    $display_number = NULL;
  }
  if ($display_type == 'default') {
    $mode = 'default';
    $display_number = 1;
  }
  $settings = weather_get_display_settings($display_type, $display_number);
  // Prevent users from entering arbitrary system-wide display numbers:
  // If the user entered a non-existant number, $settings->number will be empty.
  if ($display_type == 'system-wide' and empty($settings->number)) {
    $mode = 'add';
    $display_number = NULL;
  }
  $form['units'] = array(
   '#type' => 'fieldset',
   '#title' => t('Display units'),
   '#description' => t('Specify which units should be used for displaying the weather data.'),
   '#collapsible' => TRUE,
   '#collapsed' => FALSE,
   '#tree' => TRUE,
  );
  $form['units']['temperature'] = array(
    '#type' => 'select',
    '#title' => t('Temperature'),
    '#default_value' => $settings->units['temperature'],
    '#options' => array('celsius' => t('Celsius'), 'fahrenheit' => t('Fahrenheit'),
      'celsiusfahrenheit' => t('Celsius / Fahrenheit'),
      'fahrenheitcelsius' => t('Fahrenheit / Celsius'),
    ),
  );
  $form['units']['windspeed'] = array(
    '#type' => 'select',
    '#title' => t('Wind speed'),
    '#default_value' => $settings->units['windspeed'],
    '#options' => array('kmh' => t('km/h'), 'mph' => t('mph'), 'knots' => t('Knots'),
      'mps' => t('meter/s'), 'beaufort' => t('Beaufort'),
    ),
  );
  $form['units']['pressure'] = array(
    '#type' => 'select',
    '#title' => t('Pressure'),
    '#default_value' => $settings->units['pressure'],
    '#options' => array('hpa' => t('hPa'), 'kpa' => t('kPa'), 'inhg' => t('inHg'), 'mmhg' => t('mmHg'),
    ),
  );
  $form['units']['distance'] = array(
    '#type' => 'select',
    '#title' => t('Distance (e.g. Visibility)'),
    '#default_value' => $settings->units['distance'],
    '#options' => array('kilometers' => t('Kilometers'), 'miles' => t('UK miles')),
  );
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display settings'),
    '#description' => t('Customize the weather display.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );
  $form['settings']['data'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Data to show'),
    '#description' => t('Select which weather data should be shown in the display. Please note that some weather stations may not provide all data at all times. Therefore, some weather data might not be displayed even though it is selected.'),
    '#default_value' => $settings->settings['data'],
    '#options' => array(
      'temperature' => t('Temperature'),
      'wind' => t('Wind information'),
      'pressure' => t('Pressure'),
      'humidity' => t('Rel. Humidity'),
      'visibility' => t('Visibility'),
      'suninfo' => t('Times of sunrise and sunset'),
      'metar' => t('Original METAR report'),
    ),
  );
  $form['settings']['show_windchill'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show windchill temperature'),
    '#default_value' => $settings->settings['show_windchill'],
    '#description' => t('Calculates the temperature resulting from windchill. This is how the temperature <q>feels like</q>. Windchill temperature is only defined for temperatures below 10 °C (50 °F) and wind speeds above 5 km/h (3 mph).'),
  );
  $form['settings']['show_abbreviated_directions'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show abbreviated wind directions'),
    '#default_value' => $settings->settings['show_abbreviated_directions'],
    '#description' => t('Displays abbreviated wind directions like N, SE, or W instead of North, Southeast, or West.'),
  );
  $form['settings']['show_directions_degree'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show degrees of wind directions'),
    '#default_value' => $settings->settings['show_directions_degree'],
    '#description' => t('Displays the degrees of wind directions, e.g. North (20°).'),
  );
  $form['settings']['show_compact_block'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show compact block'),
    '#default_value' => $settings->settings['show_compact_block'],
    '#description' => t('Displays only the name, condition, and temperature of the weather station.'),
  );
  $form['type'] = array(
    '#type' => 'value',
    '#value' => $display_type,
  );
  $form['number'] = array(
    '#type' => 'value',
    '#value' => $display_number,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  // Do not show the 'delete' button if not in 'edit' mode.
  if ($mode == 'edit') {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('weather_display_delete_submit'),
    );
  }
  // Show a 'reset' button if editing the default display.
  if ($mode == 'default') {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
      '#submit' => array('weather_display_delete_confirm_submit'),
    );
  }
  return $form;
}

/**
 * Implement hook_form_submit().
 */
function weather_display_settings_form_submit($form, &$form_state) {
  if ($form_state['values']['number'] == NULL) {
    // The number does not exist, so insert a new entry.
    $used_numbers = weather_get_displays_in_use($form_state['values']['type']);
    $free_number = 1;
    foreach ($used_numbers as $number) {
      if ($number > $free_number) {
        break;
      }
      else {
        $free_number++;
      }
    }
    db_insert('weather_display')
      ->fields(array(
        'type' => $form_state['values']['type'],
        'number' => $free_number,
        'units' => serialize($form_state['values']['units']),
        'settings' => serialize($form_state['values']['settings']),
      ))
      ->execute();
  }
  else {
    // The number already exists, so overwrite the entry.
    db_merge('weather_display')
      ->key(array(
        'type' => $form_state['values']['type'],
        'number' => $form_state['values']['number']
      ))
      ->fields(array(
        'units' => serialize($form_state['values']['units']),
        'settings' => serialize($form_state['values']['settings']),
      ))
      ->execute();
  }
  if ($form_state['values']['type'] == 'user') {
    $path = 'user/' . $form_state['values']['number'] . '/weather';
  }
  else {
    $path = 'admin/config/user-interface/weather';
  }
  $form_state['redirect'] = $path;
}

/**
 * Implement hook_form_submit().
 */
function weather_display_delete_submit($form, &$form_state) {
  if ($form_state['values']['type'] == 'user') {
    $path = 'user/' . $form_state['values']['number'] . '/weather/display/delete';
  }
  else {
    $path = 'admin/config/user-interface/weather/system-wide/' . $form_state['values']['number'] . '/delete';
  }
  $form_state['redirect'] = $path;
}

/**
 * Generate a confirmation form before deleting.
 */
function weather_display_delete_confirm($form, &$form_state, $display_type, $display_number) {
  $form['type'] = array(
    '#type' => 'value',
    '#value' => $display_type,
  );
  $form['number'] = array(
    '#type' => 'value',
    '#value' => $display_number,
  );
  if ($display_type == 'user') {
    $question = t('Are you sure you want to delete your custom display?');
    $return_path = 'user/' . $display_number . '/weather';
  }
  else {
    $question = t('Are you sure you want to delete display number !number?',
      array('!number' => $display_number));
    $return_path = 'admin/config/user-interface/weather';
  }
  return confirm_form($form, $question, $return_path, NULL, t('Delete'));
}

/**
 * Implement actual deletion of display.
 */
function weather_display_delete_confirm_submit($form, &$form_state) {
  // Delete associated locations.
  db_delete('weather_location')
    ->condition('display_type', $form_state['values']['type'])
    ->condition('display_number', $form_state['values']['number'])
    ->execute();
  // Delete actual display.
  db_delete('weather_display')
    ->condition('type', $form_state['values']['type'])
    ->condition('number', $form_state['values']['number'])
    ->execute();
  if ($form_state['values']['type'] == 'user') {
    $path = 'user/' . $form_state['values']['number'] . '/weather';
  }
  else {
    $path = 'admin/config/user-interface/weather';
  }
  $form_state['redirect'] = $path;
}

/**
 * Create a settings form for a weather location.
 *
 * @param ID of the location.
 *
 * @return Form array.
 */
function weather_location_settings_form($form, &$form_state, $display_type, $display_number, $location_id=NULL) {
  $mode = 'edit';
  // Handle the addition of a new location.
  if ($location_id == 'add') {
    $mode = 'add';
    $location_id = NULL;
  }
  // If the form is regenerated during an AJAX callback, get the
  // country selected by the user.
  if (!empty($form_state['values']['country'])) {
    $settings->country = $form_state['values']['country'];
  }
  else {
    // If the location exists, get the settings. If it does not exist,
    // get the default location settings.
    $settings = weather_get_location_settings($location_id);
  }
  $settings->places = weather_get_places($settings->country);
  $form['country'] = array(
    '#type' => 'select',
    '#title' => t('Country'),
    '#description' => t('Select a country to narrow down your search.'),
    '#default_value' => $settings->country,
    '#options' => drupal_map_assoc(weather_get_countries()),
    '#ajax' => array(
      'callback' => 'weather_location_settings_form_country_callback',
      'wrapper' => 'weather_place_replace',
    ),
  );
  $form['place'] = array(
    '#type' => 'select',
    '#title' => t('Place'),
    '#description' => t('Select a place in that country for the weather display.'),
    '#default_value' => $settings->icao,
    '#options' => $settings->places,
    '#prefix' => '<div id="weather_place_replace">',
    '#suffix' => '</div>',
    '#ajax' => array(
      'callback' => 'weather_location_settings_form_place_callback',
      'wrapper' => 'weather_icao_replace',
    ),
  );
  $form['icao'] = array(
    '#type' => 'textfield',
    '#title' => t('ICAO code'),
    '#default_value' => $settings->icao,
    '#description' => t('Enter the 4-letter ICAO code of the weather station. If you first need to look up the code, you can use !url_1 or !url_2. Please note that not all stations listed at those URLs are providing weather data and thus may not be supported by this module.',
      array(
        '!url_1' => l('airlinecodes.co.uk', 'http://www.airlinecodes.co.uk/aptcodesearch.asp'),
        '!url_2' => l('notams.jcs.mil', 'https://www.notams.jcs.mil/common/icao/index.html')
      )
    ),
    '#required' => TRUE,
    '#size' => '5',
    '#prefix' => '<div id="weather_icao_replace">',
  );
  $form['real_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Real name for the selected place'),
    '#default_value' => $settings->real_name,
    '#description' => t('You may enter another name for the place selected above.'),
    '#required' => TRUE,
    '#size' => '30',
    '#suffix' => '</div>',
  );
  $form['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#default_value' => $settings->weight,
    '#description' => t('Optional. In the block, the heavier locations will sink and the lighter locations will be positioned nearer the top. Locations with equal weights are sorted alphabetically.'),
  );
  $form['id'] = array(
    '#type' => 'value',
    '#value' => $location_id,
  );
  $form['display_type'] = array(
    '#type' => 'value',
    '#value' => $display_type,
  );
  $form['display_number'] = array(
    '#type' => 'value',
    '#value' => $display_number,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  // Do not show the 'delete' button if not in 'edit' mode.
  if ($mode == 'edit') {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('weather_location_delete_submit'),
    );
  }
  if (!empty($form_state['values']['place'])) {
    $form['icao']['#value'] = $form_state['values']['place'];
    $form['real_name']['#value'] = $settings->places[$form_state['values']['place']];
  }
  return $form;
}

/**
 * AJAX callback for location settings form.
 */
function weather_location_settings_form_country_callback($form, $form_state) {
  return $form['place'];
}

/**
 * AJAX callback for location settings form.
 */
function weather_location_settings_form_place_callback($form, $form_state) {
  $ret['icao'] = $form['icao'];
  $ret['real_name'] = $form['real_name'];
  return $ret;
}

/**
 * Implement hook_form_validate().
 */
function weather_location_settings_form_validate($form, &$form_state) {
  // Make sure the ICAO code is supported by the module.
  if (weather_get_country_from_icao($form_state['values']['icao']) == '') {
    form_set_error('icao', t('The ICAO code is not supported by this module.'));
  }
}

/**
 * Implement hook_form_submit().
 */
function weather_location_settings_form_submit($form, &$form_state) {
  // Determine whether to insert or update a location.
  $location = (object) $form_state['values'];
  if ($location->id == NULL) {
    drupal_write_record('weather_location', $location);
  }
  else {
    drupal_write_record('weather_location', $location, 'id');
  }
  if ($location->display_type == 'user') {
    $form_state['redirect'] = 'user/' . $location->display_number . '/weather';
  }
  else {
    $form_state['redirect'] = 'admin/config/user-interface/weather';
  }
}

/**
 * Implement hook_form_submit().
 */
function weather_location_delete_submit($form, &$form_state) {
  if ($form_state['values']['display_type'] == 'user') {
    $path = 'user/' . $form_state['values']['display_number'] . '/weather/' .
      $form_state['values']['id'] . '/delete';
  }
  else {
    $path = 'admin/config/user-interface/weather/system-wide/' .
    $form_state['values']['display_number'] .
    '/' .
    $form_state['values']['id'] .
    '/delete';
  }
  $form_state['redirect'] = $path;
}

/**
 * Generate a confirmation form before deleting.
 */
function weather_location_delete_confirm($form, &$form_state, $location_id) {
  $location = db_query('SELECT * FROM {weather_location} WHERE id=:id',
    array(':id' => $location_id))->fetchObject();
  $form['id'] = array(
    '#type' => 'value',
    '#value' => $location_id,
  );
  $form['display_type'] = array(
    '#type' => 'value',
    '#value' => $location->display_type,
  );
  $form['display_number'] = array(
    '#type' => 'value',
    '#value' => $location->display_number,
  );
  if ($location->display_type == 'user') {
    $return_path = 'user/' . $location->display_number . '/weather';
  }
  else {
    $return_path = 'admin/config/user-interface/weather';
  }
  return confirm_form($form,
    t('Are you sure you want to delete the location %name?',
      array('%name' => $location->real_name)),
    $return_path,
    NULL,
    t('Delete')
  );
}

/**
 * Implement actual deletion of location.
 */
function weather_location_delete_confirm_submit($form, &$form_state) {
  db_delete('weather_location')
    ->condition('id', $form_state['values']['id'])
    ->execute();
  if ($form_state['values']['display_type'] == 'user') {
    $path = 'user/' . $form_state['values']['display_number'] . '/weather';
  }
  else {
    $path = 'admin/config/user-interface/weather';
  }
  $form_state['redirect'] = $path;
}

/**
 * Get all countries.
 *
 * @return Array of sorted countries.
 */
function weather_get_countries() {
  $result = db_query('SELECT country FROM {weather_icao} GROUP BY country ORDER BY country ASC');
  foreach ($result as $row) {
    $countries[] = $row->country;
  }
  return $countries;
}

/**
 * Searches for the specified location, whether it is a place name or
 * an ICAO code. For example, weather/fuhlsbüttel will display the weather
 * for Hamburg-Fuhlsbüttel.
 *
 * @param The argument passed in the URL that specifies the
 *        location which should be searched for.
 */
function weather_search_location($search = NULL) {
  if ($search == NULL) {
    // The user did not enter a search string in the URL, so just
    // display the search form.
    return drupal_get_form('weather_search_form');
  }
  else {
    $search = urldecode($search);
    // Do some sanity checks first.
    if ((strlen($search) < 3) || (strlen($search) > 64)) {
      drupal_set_message(t('The string to search for must be between 3 and 64 characters.'), 'error');
      drupal_goto('weather');
    }
    // Try to match an exact ICAO code.
    if (strlen($search) == 4) {
      $location = db_query('SELECT icao, country, name FROM {weather_icao} WHERE icao=:icao',
        array(':icao' => strtoupper($search)))->fetchObject();
      if ($location) {
        // Use the default configuration for display.
        $display = weather_get_display_settings(NULL);
        $metar = weather_get_metar($location->icao);
        $location->real_name = $location->name;
        $output = theme('weather_theming', array('display' => $display, 'location' => $location, 'metar' => $metar));
        $form = drupal_get_form('weather_search_form');
        $output .= drupal_render($form);
        return $output;
      }
    }
    // Try to match on icao, name, or country.
    $locations = array();
    $sql = "SELECT icao, country, name FROM {weather_icao}
      WHERE icao LIKE UPPER(:search)
      OR UPPER(country) LIKE UPPER(:search)
      OR UPPER(name) LIKE UPPER(:search)
      ORDER BY name ASC";
    $result = db_query($sql, array(':search' => "%$search%"));
    foreach ($result as $location) {
      $locations[] = $location;
    }

    // If there are no results, notify user
    if (empty($locations)) {
      drupal_set_message(t('Your search did not return any results.'), 'error');
      drupal_goto('weather');
    }
    else {
      if (count($locations) == 1) {
        $location = $locations[0];
        // There's only one search result, so show the weather directly
        // using the default configuration for display.
        // Use the default configuration for display.
        $display = weather_get_display_settings(NULL);
        $metar = weather_get_metar($location->icao);
        $location->real_name = $location->name;
        $output = theme('weather_theming', array('display' => $display, 'location' => $location, 'metar' => $metar));
        $form = drupal_get_form('weather_search_form');
        $output .= drupal_render($form);
        return $output;
      }
      else {
        // There is more than one result, so show all of them
        // to let the user decide.
        $links = array();
        foreach ($locations as $location) {
          $links[] = l($location->name, 'weather/' . $location->icao);
        }
        $title = t('Search results for <q>@search</q>', array('@search' => $search));
        $output = theme('item_list', array('items' => $links, 'title' => $title));
        $form = drupal_get_form('weather_search_form');
        $output .= drupal_render($form);
        return $output;
      }
    }
  }
}

/**
 * Display a form for the user to search for weather locations.
 */
function weather_search_form($form, &$form_state) {
  $form['search'] = array(
    '#type' => 'textfield',
    '#title' => t('Search for a location'),
    '#description' => t('Type in an ICAO code, a name, or a country to search for weather conditions at that location.'),
    '#autocomplete_path' => 'weather/autocomplete',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Search'),
  );
  return $form;
}

/**
 * Validate the input from the weather search form
 */
function weather_search_form_validate($form, &$form_state) {
  if ((strlen($form_state['values']['search']) < 3)
    || (strlen($form_state['values']['search']) > 64)) {
    form_set_error('search', t('The string to search for must be between 3 and 64 characters.'));
  }
}

/**
 * Submission handler for the weather search form.
 *
 * Just redirect the user to the weather URL with the search term stuffed
 * on the end of it. We've been through validation but make sure the
 * search contains no dodgy characters here.
 */
function weather_search_form_submit($form, &$form_state) {
  $form_state['redirect'] = 'weather/' . urlencode($form_state['values']['search']);
}

/**
 * Given a partial string, search for a location or ICAO code matching that
 * string.
 *
 * @param The partial text to search for.
 */
function weather_search_autocomplete($input) {
  $matches = array();
  // In this query we search for ICAO code, country, and name of locations.
  $sql = "SELECT icao, country, name FROM {weather_icao}
    WHERE icao LIKE UPPER(:input)
    OR UPPER(country) LIKE UPPER(:input)
    OR UPPER(name) LIKE UPPER(:input)
    ORDER BY name ASC";
  $result = db_query_range($sql, 0, 10, array(':input' => "%$input%"));
  foreach ($result as $match) {
    $matches[$match->icao] = check_plain(
      sprintf("%s, %s (%s)", $match->name, $match->country, $match->icao)
    );
  }
  drupal_json_output($matches);
}
