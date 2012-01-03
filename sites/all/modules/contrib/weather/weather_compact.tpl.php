<div class="weather">
  <p>
    <strong><?php print $weather->real_name; ?>:</strong>
    <?php print $weather->condition;
    if (isset($weather->temperature)) {
      print ', ' . $weather->temperature;
    }
    ?>
  <?php if (isset($weather->station)): ?>
    <small>
      <?php print t('Location of this weather station:'); ?><br />
      <?php print $weather->station; ?>
    </small>
    <br />
  <?php endif ?>
  <?php if (isset($weather->legal)): ?>
    <small>
      <?php print $weather->legal; ?>
    </small>
    <br />
  <?php endif ?>
  </p>
</div>
