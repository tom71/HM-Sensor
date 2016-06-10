package main;

use strict;
use warnings;

# device definition
$HMConfig::culHmModel{'F201'} = {name => 'HB-UW-Sen-TH-OW', st => 'THPLOWSensor', cyc => '00:10', rxt => 'w:c', lst  => 'p',   chn  => '',};
$HMConfig::culHmRegDefine{'lowBatLimitTHPL'} = {a=>18.0,s=>1.0,l=>0,min=>1.0 ,max=>5    ,c=>'',f=>10,u=>'V',  d=>0,t=>'Low batterie limit, step 0.1 V.'};

# Register model mapping
$HMConfig::culHmRegModel{'HB-UW-Sen-TH-OW'} = {
	'burstRx'         => 1,
	'lowBatLimitTHPL' => 1,
	'ledMode'         => 1,
	'transmDevTryMax' => 1,
	'altitude'        => 1
};


# subtype channel mapping
$HMConfig::culHmSubTypeSets{'THPLOWSensor'}    = {
	'peerChan'       => '0 <actChn> ... single [set|unset] [actor|remote|both]',
	'fwUpdate'       => '<filename> <bootTime> ...',
	'getSerial'      => '',
	'getVersion'     => '',
	'statusRequest'  => '',
	'burstXmit'      => ''
};

# Subtype spezific funtions    
sub CUL_HM_ParseTHPLOWSensor(@){
	
	my ($mFlg, $frameType, $src, $dst, $msgData, $targetDevIO) = @_;
	
	my $shash = CUL_HM_id2Hash($src);                                           #sourcehash - will be modified to channel entity
	my @events = ();

	# WEATHER_EVENT
	if ($frameType eq '70'){
		my $name = $shash->{NAME};
		my $chn = '01';

		my ($temp1, $temp2, $temp3, $temp4, $batVoltage) = map{hex($_)} unpack ('A4A4A4A4A4', $msgData);

		# temperature
		my $temperature =  $temp1 & 0x7fff;
		$temperature = ($temperature &0x4000) ? $temperature - 0x8000 : $temperature; 
		$temperature = sprintf('%0.1f', $temperature / 10);

		my $stateMsg = 'state:T1: ' . $temperature;
		push (@events, [$shash, 1, 'temperature:' . $temperature]);

		# battery state
		push (@events, [$shash, 1, 'battery:' . ($temp1 & 0x8000 ? 'low' : 'ok')]);

		# battery voltage
		$batVoltage = sprintf('%.2f', (($batVoltage + 0.00) / 1000));
		push (@events, [$shash, 1, 'batVoltage:' . $batVoltage]);

		# temp2
		if ($temp2)                 {
			my $temperature2 =  $temp2;
			$temperature2 = sprintf('%0.1f', $temperature2 / 10);
			$stateMsg .= ' T2: ' . $temperature2;
			push (@events, [$shash, 1, 'temperature2:' . $temperature2]);
		}
		
		# temp3
		if ($temp3)                 {
			my $temperature3 =  $temp3;
			$temperature3 = sprintf('%0.1f', $temperature3 / 10);
			$stateMsg .= ' T3: ' . $temperature3;
			push (@events, [$shash, 1, 'temperature3:' . $temperature3]);
		}
		
		# temp4
		if ($temp4)                 {
			my $temperature4 =  $temp4;
			$temperature4 = sprintf('%0.1f', $temperature4 / 10);
			$stateMsg .= ' T4: ' . $temperature4;
			push (@events, [$shash, 1, 'temperature4:' . $temperature4]);
		}
		

		push (@events, [$shash, 1, $stateMsg]);
	}

	return @events;
}

1;
