<?php
/**
 * @file
 * Contains drush hooks. Inspired by Zen and Omega Tools.
 */

/**
 * Bootswatch theme URI.
 */
define('BOOTSWATCH_URI', 'https://github.com/arshad/radix-bootswatch/archive/7.x-2.x.zip');

/**
 * Implements hook_drush_command().
 */
function oa_radix_drush_command() {
  $items = array();

  $items['oa_radix'] = array(
    'description' => 'Create a Open Atrium base theme.',
    'arguments' => array(
      'name'         => 'The name of your subtheme.',
    ),
    'options' => array(
      'machine_name' => 'The machine-readable name of your subtheme. This will be auto-generated from the human-readable name if ommited.',
      'description' => 'The description of your subtheme.',
      'destination' => 'The destination of your subtheme. Defaults to "all" (sites/all/themes).',
      'kit' => 'The name of the starter kit to use. Defaults to "default".',
      'bootswatch' => 'The Bootswatch theme to use. See https://github.com/arshad/radix-bootswatch.',
    ),
    'examples' => array(
      'drush oa_radix "My Theme"' => 'Creates a Open Atrium Radix subtheme called "My Theme", using the default options.',
      'drush oa_radix "My Theme" --machine_name=my_theme' => 'Creates a Radix subtheme called "My Theme" with a specific machine name.',
    ),
  );

  return $items;
}

/**
 * Implements hook_drush_help().
 */
function oa_radix_drush_help($section) {
  switch ($section) {
    case 'drush:oa_radix':
      return dt('This command will create a Open Atrium base theme. See examples to get started.');
  }
}

/**
 * Implements drush_hook_COMMAND().
 */
function drush_oa_radix($name = NULL, $machine_name = NULL) {
  // Determine the theme name.
  if (!isset($name)) {
    $name = drush_get_option('name');
  }

  // Determine the machine name.
  if (!isset($machine_name)) {
    $machine_name = drush_get_option('machine_name');
  }
  if (!$machine_name) {
    $machine_name = $name;
  }
  $machine_name = str_replace(' ', '_', strtolower($machine_name));
  $search = array(
    '/[^a-z0-9_]/', // Remove characters not valid in function names.
    '/^[^a-z]+/',   // Functions must begin with an alpha character.
  );
  $machine_name = preg_replace($search, '', $machine_name);

  // Determine the path to the new subtheme.
  $subtheme_path = 'sites/all/themes';
  if ($path = drush_get_option('path')) {
    $subtheme_path = drush_trim_path($path);
  }
  $subtheme_path = drush_normalize_path(drush_get_context('DRUSH_DRUPAL_ROOT') . '/' . $subtheme_path . '/' . $machine_name);

  // Determine the kit to use.
  if ($kit = drush_get_option('kit')) {
    $kit = drush_trim_path($kit);
  } else {
    $kit = 'default';
  }

  // Make a fresh copy of the kit.
  $kit_path = drush_normalize_path(drush_get_context('DRUSH_DRUPAL_ROOT') . '/' . drupal_get_path('theme', 'oa_radix') . '/kits/' . $kit);
  if (!is_dir(dirname($subtheme_path))) {
    drush_die(dt('The directory "!directory" was not found.', array('!directory' => dirname($subtheme_path))));
  }
  drush_op('drush_copy_dir', $kit_path, $subtheme_path);

  // Copy Bootswatch theme to bootswatch folder.
  if ($bootswatch = drush_get_option('bootswatch')) {
    // Switch to a temp directory.
    $current_dir = getcwd();
    chdir(drush_tempdir());

    if ($filepath = drush_download_file(BOOTSWATCH_URI)) {
      $filename = basename($filepath);
      $dirname =  basename($filepath, '.zip');

      // Decompress the zip archive.
      drush_tarball_extract($filename, $dirname, TRUE);

      // Copy bootswatch theme to subtheme.
      $bootswatch_path = $subtheme_path . '/assets/sass/bootswatch/';
      drush_move_dir($dirname . '/radix-bootswatch-master/' . $bootswatch, $bootswatch_path, TRUE);
    }

    // Set working directory back to the previous working directory.
    chdir($current_dir);
  }

  // Rename the .info file.
  $subtheme_info_file = $subtheme_path . '/' . $machine_name . '.info';
  drush_op('rename', drush_normalize_path($subtheme_path . '/' . $kit . '.info'), drush_normalize_path($subtheme_info_file));

  // Alter the contents of the .info file based on the command options.
  $alterations = array(
    '{{Name}}' => $name,
    '{{Description}}' => 'A theme based on Open Atrium Base.',
    '{{machine_name}}' => $machine_name,
    'hidden = true' => '',
  );
  if ($description = drush_get_option('description')) {
    $alterations['{{Description}}'] = $description;
  }
  drush_op('oa_radix_file_str_replace', $subtheme_info_file, array_keys($alterations), $alterations);

  // Replace all occurrences of '{{machine_name}}' with the machine name of our sub theme.
  drush_op('oa_radix_file_str_replace', $subtheme_path . '/template.php', '{{machine_name}}', $machine_name);
  drush_op('oa_radix_file_str_replace', $subtheme_path . '/theme-settings.php', '{{machine_name}}', $machine_name);

  // Notify user of the newly created theme.
  $message = dt('Open Atrium subtheme "!name" created in: !path using the "!kit" kit.', array(
    '!name' => $name,
    '!path' => $subtheme_path,
    '!kit' => $kit,
  ));
  drush_print($message);
}

/**
 * Replace strings in a file.
 */
function oa_radix_file_str_replace($file_path, $find, $replace) {
  $file_path = drush_normalize_path($file_path);
  $file_contents = file_get_contents($file_path);
  $file_contents = str_replace($find, $replace, $file_contents);
  file_put_contents($file_path, $file_contents);
}
