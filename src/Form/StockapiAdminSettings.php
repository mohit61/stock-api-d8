<?php

/**
 * @file
 * Contains \Drupal\stockapi\Form\StockapiAdminSettings.
 */

namespace Drupal\stockapi\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Render\Element;

class StockapiAdminSettings extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'stockapi_admin_settings';
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $config = $this->config('stockapi.settings');

    foreach (Element::children($form) as $variable) {
      $config->set($variable, $form_state->getValue($form[$variable]['#parents']));
    }
    $config->save();

    if (method_exists($this, '_submitForm')) {
      $this->_submitForm($form, $form_state);
    }

    parent::submitForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['stockapi.settings'];
  }

  public function buildForm(array $form, \Drupal\Core\Form\FormStateInterface $form_state) {

    $intervals = [900, 1800, 3600, 21600, 43200, 86400];

    $period = array_map([\Drupal::service('date.formatter'), 'formatInterval'], array_combine($intervals, $intervals));


    $form = [];
    $form['stockapi_fetch'] = [
      '#type' => 'select',
      '#title' => t('Stock data update frequency'),
      '#default_value' => \Drupal::config('stockapi.settings')->get('stockapi_fetch'),
      '#options' => $period,
      '#description' => t('How often to refresh the stock data from Alpha ' . 'Vantage Realtime and historical equity data. Default is 1 hour' . ' (3600 seconds).'),
    ];

    $quotetype = [
      'basic' => 'basic',
      'extended' => 'extended',
      'realtime' => 'realtime',
    ];
    $form['stockapi_quotetype'] = [
      '#type' => 'select',
      '#title' => t('Select the type of quotes to retrieve'),
      '#default_value' => \Drupal::config('stockapi.settings')->get('stockapi_quotetype'),
      '#options' => $quotetype,
      '#description' => t('Quote type: basic, extended, or realtime'),
    ];

    $form['stockapi_default_symbols'] = [
      '#type' => 'textarea',
      '#title' => t('Default symbols'),
      '#default_value' => \Drupal::config('stockapi.settings')->get('stockapi_default_symbols'),
      '#description' => t('Enter symbols to fetch quotes for, each spearated by a space'),
    ];

    $form['stockapi_decimals'] = [
      '#type' => 'textfield',
      '#title' => t('Number of decimals'),
      '#default_value' => \Drupal::config('stockapi.settings')->get('stockapi_decimals'),
      '#description' => t('Number of decimals displayed after the point.'),
      '#min' => 0,
    ];

    $form['stockapi_vantage_apikey'] = [
      '#type' => 'textfield',
      '#title' => t('The <a href="@alphavantage">API key</a> used for accessing' . ' <a href="@alphavantage">json alphavantage stock data</a>.  "demo" ' . ' is only good for stocks like AAPL and MSFT (see description).', [
        '@alphavantage' => 'https://www.alphavantage.co/support/#api-key'
        ]),
      '#default_value' => \Drupal::config('stockapi.settings')->get('stockapi_vantage_apikey'),
      '#description' => t('Please <a href="@alphavantage">register for your free stock ' . 'API key</a> demo is only good for stocks like AAPL and MSFT.  Alphavantage ' . ' asks that we not do more than one request per second.  Please do not' . ' tamper with the default request frequency settings. ' . '<a href="@termsofservice">See terms and conditions.</a>', [
        '@alphavantage' => 'https://www.alphavantage.co/support/#api-key',
        '@termsofservice' => 'https://www.alphavantage.co/terms_of_service/',
      ]),
      '#required' => TRUE,
    ];

    return parent::buildForm($form, $form_state);
  }

}
