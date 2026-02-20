-- schema.sql
-- Tabela simples para gravar as leituras do Arduino.

CREATE DATABASE IF NOT EXISTS `sensores`
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE `sensores`;

CREATE TABLE IF NOT EXISTS `leituras` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `device` VARCHAR(64) NOT NULL,
  `temp_c` DECIMAL(6,2) NOT NULL,
  `pressure_hpa` DECIMAL(8,2) NOT NULL,
  `humidity` DECIMAL(6,2) NOT NULL,
  `lux` DECIMAL(10,2) NOT NULL,
  `altitude_m` DECIMAL(8,2) NOT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_created_at` (`created_at`),
  KEY `idx_device` (`device`)
) ENGINE=InnoDB;
