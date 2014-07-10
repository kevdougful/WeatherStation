<?php
	$date=$_GET['date'];
	$con=mysqli_connect('localhost','pi','raspberry','Weather');
	$query="SELECT * FROM Weather WHERE Time LIKE '" . $date . "%'";
	$result=mysqli_query($con, $query);
	$rows = array();
	while($r = mysqli_fetch_assoc($result))
	{
		$rows[] = $r;
	}
	echo json_encode($rows);
?>