![GSoC 2018](https://img.shields.io/badge/GSoC-2018-red.svg)
![Stock API](https://img.shields.io/badge/Stock-API-orange.svg) 
![Drupal 8.x](https://img.shields.io/badge/Drupal-8.x-blue.svg) 

# [Porting Stock Api to Drupal 8](https://www.drupal.org/project/stockapi) 

## Google Summer of Code 2018 | Drupal
 
#### Student: [Mohit Malik (mohit1604)](https://www.drupal.org/u/mohit1604)
#### Mentors: - [Vaibhav Jain](https://www.drupal.org/u/vaibhavjain) | [skyred](https://www.drupal.org/u/skyredwang) | [bojanz](https://www.drupal.org/u/bojanz)

## Description:

- The [Stock Api](https://www.drupal.org/project/stockapi) module is a stock quote API module that provides other modules with a facility to incorporate stock data in them.

- Stock data retrieved from Alpha Vantage Realtime historical equity data is cached locally for performance.

- Up to 42 different fields regarding a stock can be retrieved. Each field is stored as structured data in the local database (e.g., converted to a float, varchar, etc.).

- Stock information from Alpha Vantage Realtime historical equity data is automatically updated, at an admin specified interval.

- The updates for stock information are processed one symbol at a time for however many symbols, all the symbols will be processed during a stockapi cron job.

### Installation

- Go to drupal/module directory.

- Execute the drush command `drush dl stockapi` for downloading the latest compatible version of Stock API for Drupal 8.

- Run `drush en stockapi` for installing the module.
